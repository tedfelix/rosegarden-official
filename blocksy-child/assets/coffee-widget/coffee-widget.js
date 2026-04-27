/**
 * PianoMode - Coffee Widget JavaScript
 *
 * Gère :
 * - Apparition après délai
 * - Fermeture avec localStorage
 * - Ouverture/fermeture modale
 * - Animations
 */

(function() {
    'use strict';

    // Vérifier que les settings sont chargés
    if (typeof pmCoffeeWidget === 'undefined') {
        console.warn('PM Coffee Widget: Settings not loaded');
        return;
    }

    const STORAGE_KEY = 'pm_coffee_widget_closed';
    const widget = document.getElementById('pm-coffee-widget');
    const modal = document.getElementById('pm-coffee-modal');

    if (!widget || !modal) {
        return;
    }

    // ==========================================
    // MAPPING PAYS → DEVISE PAYPAL
    // ==========================================

    const CURRENCY_MAP = {
        // Europe (EUR)
        'AT': 'EUR', 'BE': 'EUR', 'CY': 'EUR', 'EE': 'EUR', 'FI': 'EUR',
        'FR': 'EUR', 'DE': 'EUR', 'GR': 'EUR', 'IE': 'EUR', 'IT': 'EUR',
        'LV': 'EUR', 'LT': 'EUR', 'LU': 'EUR', 'MT': 'EUR', 'NL': 'EUR',
        'PT': 'EUR', 'SK': 'EUR', 'SI': 'EUR', 'ES': 'EUR',

        // Scandinavie
        'SE': 'SEK',  // Suède - Couronne suédoise
        'NO': 'NOK',  // Norvège - Couronne norvégienne
        'DK': 'DKK',  // Danemark - Couronne danoise

        // Amérique
        'US': 'USD',  // États-Unis - Dollar
        'CA': 'CAD',  // Canada - Dollar canadien
        'MX': 'MXN',  // Mexique - Peso mexicain
        'BR': 'BRL',  // Brésil - Real brésilien

        // Europe hors zone euro
        'GB': 'GBP',  // Royaume-Uni - Livre sterling
        'CH': 'CHF',  // Suisse - Franc suisse
        'PL': 'PLN',  // Pologne - Zloty
        'CZ': 'CZK',  // République tchèque - Couronne tchèque
        'HU': 'HUF',  // Hongrie - Forint
        'RO': 'RON',  // Roumanie - Leu
        'RU': 'RUB',  // Russie - Rouble
        'TR': 'TRY',  // Turquie - Livre turque

        // Asie
        'CN': 'CNY',  // Chine - Yuan
        'JP': 'JPY',  // Japon - Yen
        'IN': 'INR',  // Inde - Roupie indienne
        'KR': 'KRW',  // Corée du Sud - Won
        'SG': 'SGD',  // Singapour - Dollar singapourien
        'TH': 'THB',  // Thaïlande - Baht
        'VN': 'VND',  // Vietnam - Dong
        'ID': 'IDR',  // Indonésie - Roupie indonésienne
        'PH': 'PHP',  // Philippines - Peso philippin
        'MY': 'MYR',  // Malaisie - Ringgit
        'TW': 'TWD',  // Taïwan - Dollar taïwanais
        'HK': 'HKD',  // Hong Kong - Dollar de Hong Kong

        // Moyen-Orient & Afrique du Nord
        'AE': 'AED',  // Émirats - Dirham
        'SA': 'SAR',  // Arabie Saoudite - Riyal
        'IL': 'ILS',  // Israël - Shekel
        'MA': 'MAD',  // Maroc - Dirham marocain
        'TN': 'TND',  // Tunisie - Dinar tunisien
        'EG': 'EGP',  // Égypte - Livre égyptienne
        'DZ': 'DZD',  // Algérie - Dinar algérien

        // Autres
        'AU': 'AUD',  // Australie - Dollar australien
        'NZ': 'NZD',  // Nouvelle-Zélande - Dollar néo-zélandais
        'ZA': 'ZAR',  // Afrique du Sud - Rand

        // Défaut
        'DEFAULT': 'USD'
    };

    // Détection du pays utilisateur
    let detectedCountry = null;
    let detectedCurrency = 'USD';

    async function detectUserCountry() {
        try {
            // 1. Vérifier si force_country est dans l'URL (pour les tests)
            const urlParams = new URLSearchParams(window.location.search);
            const forcedCountry = urlParams.get('force_country');

            if (forcedCountry) {
                detectedCountry = forcedCountry.toUpperCase();
                detectedCurrency = CURRENCY_MAP[detectedCountry] || CURRENCY_MAP['DEFAULT'];
                updatePayPalCurrency(detectedCurrency);
                return detectedCountry;
            }

            // 2. Geo-detect country (try multiple CORS-friendly services)
            var geoData = null;
            var geoServices = [
                { url: 'https://api.country.is/', key: 'country' },
                { url: 'https://ipwho.is/', key: 'country_code' },
            ];

            for (var i = 0; i < geoServices.length; i++) {
                try {
                    var geoResp = await fetch(geoServices[i].url);
                    if (geoResp.ok) {
                        geoData = await geoResp.json();
                        if (geoData && geoData[geoServices[i].key]) {
                            detectedCountry = geoData[geoServices[i].key];
                            break;
                        }
                    }
                } catch (e) { /* try next */ }
            }

            if (detectedCountry) {
                detectedCurrency = CURRENCY_MAP[detectedCountry] || CURRENCY_MAP['DEFAULT'];


                // Mettre à jour le formulaire PayPal
                updatePayPalCurrency(detectedCurrency);

                return detectedCountry;
            }
        } catch (error) {
            console.warn('PM Coffee Widget: Could not detect country, using USD', error);
            detectedCurrency = 'USD';
        }

        return null;
    }

    function updatePayPalCurrency(currency) {
        const currencyInput = modal.querySelector('input[name="currency_code"]');
        if (currencyInput) {
            currencyInput.value = currency;
        }
    }

    // ==========================================
    // 1. VÉRIFIER SI L'UTILISATEUR A FERMÉ
    // ==========================================

    function isWidgetClosed() {
        return localStorage.getItem(STORAGE_KEY) === 'true';
    }

    function closeWidget() {
        localStorage.setItem(STORAGE_KEY, 'true');
        widget.classList.add('pm-coffee-fade-out');
        setTimeout(() => {
            widget.style.display = 'none';
        }, 300);
    }

    // ==========================================
    // 2. AFFICHER LE WIDGET APRÈS DÉLAI
    // ==========================================

    function showWidget() {
        // Ne pas afficher si déjà fermé
        if (isWidgetClosed()) {
            return;
        }

        setTimeout(() => {
            widget.style.display = '';
            widget.classList.remove('pm-coffee-hidden');
            widget.classList.add('pm-coffee-slide-in');
        }, pmCoffeeWidget.delay);
    }

    // ==========================================
    // 3. GESTION MODALE
    // ==========================================

    function openModal() {
        modal.classList.remove('pm-coffee-hidden');
        modal.classList.add('pm-coffee-modal-open');
        document.body.style.overflow = 'hidden'; // Empêcher scroll
    }

    function closeModal() {
        modal.classList.remove('pm-coffee-modal-open');
        modal.classList.add('pm-coffee-modal-close');
        document.body.style.overflow = ''; // Restaurer scroll

        setTimeout(() => {
            modal.classList.add('pm-coffee-hidden');
            modal.classList.remove('pm-coffee-modal-close');
        }, 600);  // 600ms pour fermeture ultra douce
    }

    // ==========================================
    // 4. EVENT LISTENERS
    // ==========================================

    // Bouton principal : ouvrir modale
    const coffeeBtn = widget.querySelector('.pm-coffee-btn');
    if (coffeeBtn) {
        coffeeBtn.addEventListener('click', (e) => {
            e.preventDefault();
            openModal();
        });
    }

    // Bouton fermer widget
    const closeBtn = widget.querySelector('.pm-coffee-close');
    if (closeBtn) {
        closeBtn.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            closeWidget();
        });
    }

    // Bouton fermer modale
    const modalCloseBtn = modal.querySelector('.pm-coffee-modal-close');
    if (modalCloseBtn) {
        modalCloseBtn.addEventListener('click', (e) => {
            e.preventDefault();
            closeModal();
        });
    }

    // Cliquer sur l'overlay pour fermer
    const overlay = modal.querySelector('.pm-coffee-modal-overlay');
    if (overlay) {
        overlay.addEventListener('click', () => {
            closeModal();
        });
    }

    // Fermer modale avec Escape
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && !modal.classList.contains('pm-coffee-hidden')) {
            closeModal();
        }
    });

    // ==========================================
    // 5. INITIALISATION
    // ==========================================

    // Afficher le widget après le délai
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', () => {
            showWidget();
            detectUserCountry();  // Détecter le pays pour la bonne devise PayPal
        });
    } else {
        showWidget();
        detectUserCountry();  // Détecter le pays pour la bonne devise PayPal
    }

    // bfcache fix: when the browser restores a page from its back-forward
    // cache the JS doesn't re-run, so any open modal/widget stays visible.
    // Reset everything to a clean state on pageshow if the page was cached.
    window.addEventListener('pageshow', function(e) {
        if (!e.persisted) return;
        // Close the donation modal if it was open when the user navigated away
        if (modal) {
            modal.classList.remove('pm-coffee-modal-open', 'pm-coffee-modal-close');
            modal.classList.add('pm-coffee-hidden');
        }
        // Restore scroll in case it was locked
        document.body.style.overflow = '';
        // If widget was closed (localStorage) but the bfcache state shows it open, re-hide it
        if (widget && isWidgetClosed()) {
            widget.style.display = 'none';
        }
    });

    // Debug info
    if (window.location.search.includes('debug=coffee')) {
            delay: pmCoffeeWidget.delay,
            isClosed: isWidgetClosed(),
            modalTitle: pmCoffeeWidget.modalTitle,
            paypalEmail: pmCoffeeWidget.paypalEmail
        });
    }

})();