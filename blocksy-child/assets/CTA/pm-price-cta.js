/**
 * PianoMode Price CTA - Version 8.0.0 (PRODUCTION)
 * ----------------------------------------------------------------------
 * Système de routing géographique pour liens d'affiliation Amazon
 *
 * MARCHÉS ONELINK : CA, FR, DE, IT, NL, PL, ES, SE, UK
 *   → Sans lien direct: utilisent US → OneLink redirige automatiquement
 *
 * MARCHÉS HORS ONELINK (liens manuels requis) : AU, IE, BE, IN, AE
 *   → DOIVENT avoir des liens directs configurés par produit
 *   → Sans lien: fallback US (commission US, pas locale)
 *
 * CORRECTIONS v7.9.0 - FIX CRITIQUE TRACKING + TAGS NON-ONELINK :
 *
 * - FIX CRITIQUE #1: getOptimalLink retourne maintenant { url, resolvedCountry }
 *   Le pays résolu est stocké sur le bouton (data-pm-resolved-country) à l'init.
 *   Le click handler utilise cette valeur au lieu de re-détecter depuis l'URL.
 *   AVANT: pour les liens amzn.to, getTargetCountryFromLink retournait 'US'
 *   (car amzn.to ne contient pas amazon.com.au), puis le code "corrigeait"
 *   en remappant vers le pays du visiteur - y compris les pays hors OneLink.
 *   APRÈS: le pays cible est déterminé à l'init quand on SAIT quel slot
 *   pays a été choisi, et stocké sur le bouton pour le click handler.
 *
 * - FIX CRITIQUE #2: addAmazonTag n'applique PLUS le tag US en "safety net"
 *   sur les domaines Amazon non-US hors OneLink (amazon.com.au, amazon.in, etc.)
 *   Un tag US sur amazon.com.au = Amazon IGNORE le tag → 0 commission.
 *   Maintenant: WARNING explicite si le tag local n'est pas configuré.
 *
 * - FIX #3: Click handler simplifié - utilise data-pm-resolved-country
 *   au lieu du système fragile de remap amazonLocalStores.
 *
 * CORRECTIONS v7.8.0 - ROBUSTESSE & TRACKING FIX :
 * - FIX CRITIQUE: Click handler no longer mutates module-level currentCountry
 *   (was permanently changing 'GB' to 'UK' after first click, causing state bugs)
 * - FIX: getTargetCountryFromLink returns 'UK' for amazon.co.uk (was 'GB')
 * - FIX: SE/PL/Scandinavian/Eastern EU countries now show EUR prices (was USD)
 * - NEW: Click-time URL validation prevents navigation to blank pages
 * - NEW: Strengthened fallback chain in getOptimalLink (checks all regions)
 * - NEW: Safety net in initSystem restores PHP fallback if JS link resolution fails
 * - FIX: GB/UK normalization in tracking uses local variables, not module state
 *
 * CORRECTIONS v7.5.0 - DISTINCTION ONELINK vs HORS-ONELINK :
 * - NOUVEAU: Liste ONELINK_COUNTRIES pour les pays couverts par OneLink
 * - NOUVEAU: isOneLinkCountry() pour vérifier si un pays est dans OneLink
 * - FIX: Les pays HORS OneLink (AU, IE, BE, IN, AE) ne vont plus vers US
 *   aveuglément. Ils utilisent un fallback géographique approprié.
 * - FIX: Seuls les pays DANS OneLink utilisent le lien US pour redirection
 * - WARNING logs pour les pays hors OneLink sans lien direct
 *
 * CORRECTIONS v7.4.0 - ONELINK FIX CRITIQUE :
 * - FIX: Les pays avec un ID Amazon configuré mais sans lien direct
 *   utilisent maintenant le lien US pour que OneLink redirige vers
 *   le store local. AVANT: SE/PL → DE (FAUX). APRÈS: SE/PL → US → OneLink → amazon.se/pl
 *
 * NOUVEAUTÉS v7.3.0 :
 * - Ajout support complet Suède (SE) et Pologne (PL) via OneLink
 * - Ajout détection amazon.se et amazon.pl dans getTargetCountryFromLink
 * - Ajout SE/PL dans tagKeyMap pour attribution correcte des commissions
 * - Compatible avec Amazon OneLink pour redirection automatique
 *
 * CORRECTIONS v7.2.2 :
 * - FIX CRITIQUE UK/CA/ALL: sendBeacon n'enregistrait AUCUN clic car
 *   WordPress AJAX nécessite 'action' dans l'URL pour router la requête.
 *   sendBeacon envoyait les données dans php://input, mais WordPress
 *   cherche $_REQUEST['action'] AVANT d'appeler le handler.
 *   SOLUTION: ?action=pm_track_click ajouté à l'URL sendBeacon.
 *
 * CORRECTIONS v7.2.1 :
 * - FIX CRITIQUE: target_country utilise maintenant le pays VISITEUR
 *   quand Amazon a un store local (FR, CA, UK, DE, etc.)
 *   Raison: Amazon redirige automatiquement vers le store local,
 *   donc la commission est attribuée au store du visiteur, pas US.
 * - Ajout fallback FR → DE → US quand link_fr n'existe pas
 *
 * CORRECTIONS v7.2 :
 * - FIX: Tracking du store Amazon destination (FR/CA au lieu de US)
 * - FIX: Lecture du data-link-* pour déterminer le vrai pays cible
 * - Amélioration du logging pour debug
 *
 * CORRECTIONS v7.1 :
 * - Tracking enregistre maintenant le pays VISITEUR + pays DESTINATION
 * - Ajout de Singapour, Japon, Mexique, Brésil
 * - Fallbacks améliorés pour tous les pays asiatiques et LATAM
 * - sendBeacon avec fallback fetch robuste
 * - Debug amélioré avec logs détaillés
 * ----------------------------------------------------------------------
 */

(function() {
    'use strict';

    // ==========================================
    // 0. CONFIGURATION & DEBUG
    // ==========================================

    const DEBUG = (typeof pmPriceCTA !== 'undefined' && pmPriceCTA.debug) ||
                  window.location.search.includes('pm_debug=1');

    function log(...args) {
        if (DEBUG) {
            console.log('[PM CTA]', ...args);
        }
    }

    // ==========================================
    // 1. DÉTECTION GÉOGRAPHIQUE
    // ==========================================

    async function detectCountry() {
        // Mode debug forcé via URL
        const urlParams = new URLSearchParams(window.location.search);
        const forced = urlParams.get('force_country');
        if (forced) {
            localStorage.removeItem('pm_geo_country');
            log('Country forced via URL:', forced.toUpperCase());
            return forced.toUpperCase();
        }

        // Cache local (1 heure)
        try {
            const cached = JSON.parse(localStorage.getItem('pm_geo_country'));
            if (cached && Date.now() < cached.expiry) {
                log('Country from cache:', cached.value);
                return cached.value;
            }
        } catch(e) {}

        // Appel API via PHP (ipinfo.io)
        if (typeof pmPriceCTA !== 'undefined' && pmPriceCTA.ajaxUrl) {
            try {
                const res = await fetch(pmPriceCTA.ajaxUrl, {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: `action=pm_get_geo&nonce=${pmPriceCTA.nonce}`
                });
                const data = await res.json();

                if (data.success && data.data.country) {
                    let country = data.data.country.toUpperCase();

                    // Normaliser GB → GB (ipinfo retourne GB, pas UK)
                    // On garde GB en interne mais on mappe vers UK pour les liens

                    localStorage.setItem('pm_geo_country', JSON.stringify({
                        value: country,
                        expiry: Date.now() + 3600000 // 1 heure
                    }));

                    log('Country from API:', country);
                    return country;
                }
            } catch(e) {
                log('API error, falling back to timezone:', e);
            }
        }

        // Fallback Timezone
        const tzCountry = detectFromTimezone();
        log('Country from timezone:', tzCountry);
        return tzCountry;
    }

    function detectFromTimezone() {
        try {
            const tz = Intl.DateTimeFormat().resolvedOptions().timeZone || '';

            const tzMap = {
                // AMÉRIQUE DU NORD
                'America/New_York': 'US', 'America/Chicago': 'US', 'America/Denver': 'US',
                'America/Los_Angeles': 'US', 'America/Phoenix': 'US', 'America/Detroit': 'US',
                'America/Indiana/Indianapolis': 'US', 'America/Anchorage': 'US',
                'America/Honolulu': 'US', 'Pacific/Honolulu': 'US',
                'America/Toronto': 'CA', 'America/Vancouver': 'CA', 'America/Montreal': 'CA',
                'America/Edmonton': 'CA', 'America/Winnipeg': 'CA', 'America/Halifax': 'CA',
                'America/St_Johns': 'CA', 'America/Regina': 'CA',

                // ROYAUME-UNI & IRLANDE
                'Europe/London': 'GB',
                'Europe/Belfast': 'GB',
                'Europe/Dublin': 'IE',

                // EUROPE
                'Europe/Paris': 'FR',
                'Europe/Berlin': 'DE',
                'Europe/Vienna': 'AT',
                'Europe/Zurich': 'CH',
                'Europe/Rome': 'IT',
                'Europe/Madrid': 'ES',
                'Atlantic/Canary': 'ES',
                'Europe/Amsterdam': 'NL',
                'Europe/Brussels': 'BE',
                'Europe/Lisbon': 'PT',
                'Europe/Luxembourg': 'LU',

                // SCANDINAVIE & EUROPE DU NORD
                'Europe/Stockholm': 'SE',
                'Europe/Oslo': 'NO',
                'Europe/Copenhagen': 'DK',
                'Europe/Helsinki': 'FI',

                // EUROPE CENTRALE & EST
                'Europe/Warsaw': 'PL',
                'Europe/Prague': 'CZ',
                'Europe/Budapest': 'HU',
                'Europe/Bucharest': 'RO',
                'Europe/Sofia': 'BG',
                'Europe/Athens': 'GR',

                // OCÉANIE
                'Australia/Sydney': 'AU',
                'Australia/Melbourne': 'AU',
                'Australia/Brisbane': 'AU',
                'Australia/Perth': 'AU',
                'Australia/Adelaide': 'AU',
                'Australia/Darwin': 'AU',
                'Australia/Hobart': 'AU',
                'Australia/Canberra': 'AU',
                'Pacific/Auckland': 'NZ',

                // ASIE
                'Asia/Dubai': 'AE',
                'Asia/Kolkata': 'IN',
                'Asia/Mumbai': 'IN',
                'Asia/Tokyo': 'JP',
                'Asia/Singapore': 'SG',
                'Asia/Hong_Kong': 'HK',
                'Asia/Shanghai': 'CN',
                'Asia/Seoul': 'KR'
            };

            if (tzMap[tz]) return tzMap[tz];

            // Fallback par préfixe
            if (tz.startsWith('America/')) return 'US';
            if (tz.startsWith('Australia/')) return 'AU';
            if (tz.startsWith('Europe/')) return 'FR'; // Europe → France par défaut

        } catch(e) {}

        return 'US'; // Fallback ultime
    }

    // ==========================================
    // 2. SÉLECTION DU LIEN OPTIMAL
    // ==========================================

    /**
     * Liste des pays couverts par OneLink (configuré dans Amazon Associates)
     * Ces pays peuvent utiliser le lien US car OneLink redirige automatiquement
     * IMPORTANT: Cette liste doit correspondre à ta config OneLink dans Amazon!
     */
    const ONELINK_COUNTRIES = [
        'CA', 'FR', 'DE', 'IT', 'NL', 'PL', 'ES', 'SE', 'GB', 'UK'
    ];

    /**
     * Vérifie si un pays est couvert par OneLink
     */
    function isOneLinkCountry(countryCode) {
        // Normaliser GB/UK
        const normalized = countryCode === 'UK' ? 'GB' : countryCode;
        return ONELINK_COUNTRIES.includes(normalized) || ONELINK_COUNTRIES.includes(countryCode);
    }

    /**
     * Vérifie si un pays a un ID Amazon configuré dans les settings
     */
    function hasConfiguredAmazonId(countryCode) {
        if (!pmPriceCTA || !pmPriceCTA.amazonIds) return false;

        // Mapper le code pays vers la clé de l'ID
        const countryToKey = {
            'US': 'us', 'CA': 'ca', 'GB': 'uk', 'UK': 'uk',
            'FR': 'fr', 'DE': 'de', 'IT': 'it', 'ES': 'es',
            'NL': 'nl', 'BE': 'be', 'AU': 'au', 'IN': 'in',
            'IE': 'ie', 'AE': 'ae', 'SG': 'sg', 'JP': 'jp',
            'MX': 'mx', 'BR': 'br', 'SE': 'se', 'PL': 'pl'
        };

        const key = countryToKey[countryCode];
        if (!key) return false;

        const id = pmPriceCTA.amazonIds[key];
        return id && id.trim() !== '';
    }

    function getOptimalLink(button, country) {
        const ds = button.dataset;
        const storeType = ds.store || 'amazon';

        log('getOptimalLink() - Country:', country, 'Store:', storeType);

        // SWEETWATER (US uniquement)
        if (storeType === 'sweetwater') {
            return { url: ds.linkSweetwater || button.href, resolvedCountry: 'US' };
        }

        // SAM ASH (US uniquement)
        if (storeType === 'samash') {
            return { url: ds.linkSamash || button.href, resolvedCountry: 'US' };
        }

        // GEAR4MUSIC (UK/EU - lien unique mondial)
        if (storeType === 'gear4music') {
            return { url: ds.linkGear4music || button.href, resolvedCountry: country };
        }

        // BEATPORT (lien unique mondial)
        if (storeType === 'beatport') {
            return { url: ds.linkBeatport || button.href, resolvedCountry: country };
        }

        // THOMANN
        if (storeType === 'thomann') {
            if (['US', 'CA', 'AU', 'NZ'].includes(country)) {
                return { url: ds.linkUs || ds.linkEu || button.href, resolvedCountry: 'US' };
            }
            return { url: ds.linkEu || ds.linkUs || button.href, resolvedCountry: country };
        }

        // ==========================================
        // AMAZON - LOGIQUE DE FALLBACK CORRIGÉE
        // ==========================================

        // Étape 1 : Mapping direct pays → lien
        const directLinks = {
            'US': ds.linkUs,
            'CA': ds.linkCa,
            'GB': ds.linkUk,  // ipinfo retourne GB
            'UK': ds.linkUk,  // Au cas où
            'FR': ds.linkFr,
            'DE': ds.linkDe,
            'IT': ds.linkIt,
            'ES': ds.linkEs,
            'NL': ds.linkNl,
            'SE': ds.linkSe,  // Suède (OneLink)
            'PL': ds.linkPl,  // Pologne (OneLink)
            'BE': ds.linkBe,
            'AU': ds.linkAu,
            'NZ': ds.linkAu,  // Nouvelle-Zélande → Australie
            'IN': ds.linkIn,
            'IE': ds.linkIe,
            'AE': ds.linkAe,
            'SG': ds.linkSg,  // Singapour
            'JP': ds.linkJp,  // Japon
            'MX': ds.linkMx,  // Mexique
            'BR': ds.linkBr,  // Brésil
        };

        let selectedLink = directLinks[country];
        let usedFallback = false;
        let fallbackReason = '';

        // ================================================================
        // ÉTAPE 2 : FALLBACK INTELLIGENT
        // ================================================================
        //
        // RÈGLE 1: Pays dans OneLink (CA, FR, DE, IT, NL, PL, ES, SE, UK)
        //   → Si pas de lien direct, utiliser US → OneLink redirige
        //
        // RÈGLE 2: Pays HORS OneLink mais avec compte (AU, IE, BE, IN, AE)
        //   → Si pas de lien direct, fallback géographique approprié
        //   → L'utilisateur DOIT configurer les liens manuellement pour ces pays
        //
        // RÈGLE 3: Pays sans compte Amazon
        //   → Fallback vers le store le plus pertinent géographiquement
        // ================================================================

        if (!selectedLink) {
            usedFallback = true;

            // PRIORITÉ 1: Pays dans OneLink → US pour redirection automatique
            if (isOneLinkCountry(country)) {
                selectedLink = ds.linkUs;
                fallbackReason = `${country} in OneLink - using US for automatic redirection`;
                log('OneLink mode:', country, '→ US link (OneLink will redirect to local store)');
            }
            // PRIORITÉ 2: Pays HORS OneLink mais avec compte Amazon configuré
            // Ces pays nécessitent des liens directs, sinon fallback géographique
            else if (country === 'AU') {
                // Australie → pas dans OneLink, fallback US (même langue)
                // Le visiteur atterrit sur amazon.com avec tag US → commission US (pas AU)
                // Pour gagner des commissions AU, il FAUT configurer un lien AU direct
                selectedLink = ds.linkUs;
                fallbackReason = 'AU (not in OneLink) → US fallback (commission US, not AU)';
                log('WARNING: AU not in OneLink, using US link. To earn AU commissions, add a direct AU link!');
            }
            else if (country === 'IE') {
                // Irlande → pas dans OneLink, fallback UK (proche géographiquement)
                selectedLink = ds.linkUk || ds.linkUs;
                fallbackReason = 'IE (not in OneLink) → UK fallback - configure IE links for IE commissions!';
                log('WARNING: IE not in OneLink, using UK. Configure direct IE links to earn IE commissions!');
            }
            else if (country === 'BE') {
                // Belgique → pas dans OneLink, fallback FR ou NL (voisins)
                selectedLink = ds.linkFr || ds.linkNl || ds.linkUs;
                fallbackReason = 'BE (not in OneLink) → FR/NL fallback - configure BE links for BE commissions!';
                log('WARNING: BE not in OneLink, using FR/NL. Configure direct BE links to earn BE commissions!');
            }
            else if (country === 'IN') {
                // Inde → pas dans OneLink, fallback US
                selectedLink = ds.linkUs;
                fallbackReason = 'IN (not in OneLink) → US fallback - configure IN links for IN commissions!';
                log('WARNING: IN not in OneLink, using US. Configure direct IN links to earn IN commissions!');
            }
            else if (country === 'AE') {
                // UAE → pas dans OneLink, fallback US
                selectedLink = ds.linkUs;
                fallbackReason = 'AE (not in OneLink) → US fallback - configure AE links for AE commissions!';
                log('WARNING: AE not in OneLink, using US. Configure direct AE links to earn AE commissions!');
            }
            // PRIORITÉ 3: Pays SANS compte Amazon → fallback géographique
            else if (['AT', 'CH'].includes(country)) {
                // Autriche/Suisse germanophones → DE
                selectedLink = ds.linkDe || ds.linkUs;
                fallbackReason = `${country} (no account) → DE fallback`;
            }
            else if (['LU'].includes(country)) {
                // Luxembourg → FR ou DE
                selectedLink = ds.linkFr || ds.linkDe || ds.linkUs;
                fallbackReason = `${country} (no account) → FR/DE fallback`;
            }
            else if (['CZ', 'HU', 'RO', 'BG', 'SK', 'HR', 'SI'].includes(country)) {
                // Europe de l'Est sans compte → DE
                selectedLink = ds.linkDe || ds.linkUs;
                fallbackReason = `Eastern Europe (${country}) → DE fallback`;
            }
            else if (['NO', 'DK', 'FI'].includes(country)) {
                // Scandinavie sans compte → SE si dispo, sinon DE, sinon US
                selectedLink = ds.linkSe || ds.linkDe || ds.linkUs;
                fallbackReason = `Nordic (${country}) → SE/DE fallback`;
            }
            else if (['PT', 'GR', 'CY', 'MT'].includes(country)) {
                // Sud Europe sans compte → ES ou FR
                selectedLink = ds.linkEs || ds.linkFr || ds.linkUs;
                fallbackReason = `South Europe (${country}) → ES/FR fallback`;
            }
            else if (['HK', 'TH', 'MY', 'PH', 'KR', 'TW', 'CN', 'ID', 'VN'].includes(country)) {
                // Asie sans compte → US
                selectedLink = ds.linkUs;
                fallbackReason = `Asia (${country}) → US fallback`;
            }
            else if (['AR', 'CL', 'CO', 'PE', 'VE', 'EC', 'UY', 'PY', 'BO'].includes(country)) {
                // Amérique Latine sans compte → US
                selectedLink = ds.linkUs;
                fallbackReason = `LATAM (${country}) → US fallback`;
            }
            else if (['SA', 'OM', 'QA', 'KW', 'BH', 'JO', 'LB'].includes(country)) {
                // Moyen-Orient sans compte → AE si dispo, sinon US
                selectedLink = ds.linkAe || ds.linkUs;
                fallbackReason = `Middle East (${country}) → AE/US fallback`;
            }
            else if (['DZ', 'MA', 'TN', 'SN', 'CI'].includes(country)) {
                // Afrique francophone → FR
                selectedLink = ds.linkFr || ds.linkUs;
                fallbackReason = `Africa FR (${country}) → FR fallback`;
            }
            else if (['ZA', 'NG', 'KE', 'EG'].includes(country)) {
                // Afrique anglophone → UK ou US
                selectedLink = ds.linkUk || ds.linkUs;
                fallbackReason = `Africa EN (${country}) → UK/US fallback`;
            }
            else if (country === 'NZ') {
                // Nouvelle-Zélande → AU si dispo, sinon US
                selectedLink = ds.linkAu || ds.linkUs;
                fallbackReason = 'NZ → AU/US fallback';
            }
            // FRANCE sans lien FR → DE → US
            else if (country === 'FR') {
                selectedLink = ds.linkDe || ds.linkUs;
                fallbackReason = 'FR → DE/US fallback';
            }
            // ALLEMAGNE sans lien DE → FR → US
            else if (country === 'DE') {
                selectedLink = ds.linkFr || ds.linkUs;
                fallbackReason = 'DE → FR/US fallback';
            }
            // UK sans lien UK → US
            else if (country === 'UK') {
                selectedLink = ds.linkUs;
                fallbackReason = 'UK → US fallback';
            }
        }

        // Étape 3 : Fallback ultime si toujours rien
        if (!selectedLink) {
            selectedLink = ds.linkUs || ds.linkUk || ds.linkFr || ds.linkDe ||
                           ds.linkCa || ds.linkIt || ds.linkEs || ds.linkAu ||
                           button.href;
            fallbackReason = 'Ultimate fallback → first available';
            usedFallback = true;
        }

        // SAFETY NET: Never return empty/undefined - always fall back to button.href
        if (!selectedLink) {
            log('CRITICAL: No link found at all, keeping original href');
            selectedLink = button.href;
        }

        // Déterminer le VRAI pays cible (crucial pour les liens amzn.to
        // dont on ne peut pas extraire le pays depuis l'URL)
        let resolvedCountry;
        if (!usedFallback) {
            // Lien trouvé dans directLinks pour ce pays
            const isShortened = selectedLink.includes('amzn.to') || selectedLink.includes('amzn.eu') || selectedLink.includes('amzn.asia');
            if (isShortened) {
                // Lien raccourci : impossible de détecter le pays depuis l'URL
                // On fait confiance au slot pays configuré (c'est le mieux qu'on puisse faire)
                resolvedCountry = country;
            } else {
                // URL complète : détecter le VRAI pays cible depuis le domaine
                // Corrige le cas NZ→AU (directLinks mappe NZ vers ds.linkAu)
                // et tout autre mapping cross-pays
                resolvedCountry = getTargetCountryFromLink(selectedLink);
            }
            log('Direct link found for', country, '→ resolved target:', resolvedCountry);
        } else {
            log('Fallback used:', fallbackReason, '→', selectedLink?.substring(0, 50) + '...');
            if (isOneLinkCountry(country)) {
                // OneLink: même avec fallback US, OneLink redirigera vers le store local
                // ET convertira le tag → target = pays du visiteur
                resolvedCountry = country;
            } else {
                // Hors OneLink: le target est le pays de l'URL sélectionnée
                // (pas le pays du visiteur, car sans OneLink pas de redirection garantie)
                resolvedCountry = getTargetCountryFromLink(selectedLink);
            }
        }

        // Normaliser GB → UK
        if (resolvedCountry === 'GB') resolvedCountry = 'UK';

        return { url: selectedLink, resolvedCountry: resolvedCountry };
    }

    // ==========================================
    // 3. DÉTERMINER LE PAYS CIBLE DU LIEN
    // ==========================================

    function getTargetCountryFromLink(url) {
        if (!url) return 'US';

        // Ordre important : les URLs les plus spécifiques d'abord (domaines composés)
        if (url.includes('amazon.com.au')) return 'AU';
        if (url.includes('amazon.com.be')) return 'BE';
        if (url.includes('amazon.com.br')) return 'BR';
        if (url.includes('amazon.com.mx')) return 'MX';
        if (url.includes('amazon.com.sg')) return 'SG';
        if (url.includes('amazon.co.uk')) return 'UK';
        if (url.includes('amazon.co.jp')) return 'JP';
        // Domaines simples
        if (url.includes('amazon.ca')) return 'CA';
        if (url.includes('amazon.fr')) return 'FR';
        if (url.includes('amazon.de')) return 'DE';
        if (url.includes('amazon.it')) return 'IT';
        if (url.includes('amazon.es')) return 'ES';
        if (url.includes('amazon.nl')) return 'NL';
        if (url.includes('amazon.in')) return 'IN';
        if (url.includes('amazon.ie')) return 'IE';
        if (url.includes('amazon.ae')) return 'AE';
        if (url.includes('amazon.sg')) return 'SG';
        if (url.includes('amazon.se')) return 'SE';  // Suède
        if (url.includes('amazon.pl')) return 'PL';  // Pologne
        if (url.includes('amazon.com')) return 'US'; // .com en dernier (le plus générique)

        return 'US';
    }

    // ==========================================
    // 4. AJOUT TAG AMAZON AFFILIATE
    // ==========================================

    function addAmazonTag(url, storeType) {
        if (storeType !== 'amazon' || !url) return url;

        // Liens raccourcis amzn.to : le tag est embarqué dans la redirection Amazon, pas modifiable côté client
        if (url.includes('amzn.to') || url.includes('amzn.eu') || url.includes('amzn.asia')) {
            log('Shortened Amazon URL (' + url.substring(0, 35) + '...) - using embedded tag from redirect');
            return url;
        }

        if (!url.includes('amazon.')) return url;

        // Déterminer le pays cible basé sur l'URL du lien
        const targetCountry = getTargetCountryFromLink(url);

        // Mapper vers la clé du tag - TOUS les marchés Amazon supportés
        const tagKeyMap = {
            'US': 'us', 'CA': 'ca', 'GB': 'uk', 'UK': 'uk',
            'FR': 'fr', 'DE': 'de', 'IT': 'it', 'ES': 'es',
            'NL': 'nl', 'BE': 'be', 'AU': 'au', 'IN': 'in',
            'IE': 'ie', 'AE': 'ae', 'SG': 'sg', 'JP': 'jp',
            'MX': 'mx', 'BR': 'br',
            'SE': 'se', 'PL': 'pl'  // Suède et Pologne (OneLink)
        };

        const tagKey = tagKeyMap[targetCountry];

        if (!pmPriceCTA || !pmPriceCTA.amazonIds) {
            log('Warning: pmPriceCTA.amazonIds not configured');
            return url;
        }

        const tag = pmPriceCTA.amazonIds[tagKey];

        if (!tag || tag === '') {
            // FIX v7.7: Pour les pays OneLink, utiliser le tag US en fallback
            // OneLink convertira automatiquement le tag US vers le tag local côté Amazon
            // SANS CE FIX: les URLs complètes (amazon.co.uk, amazon.ca) étaient servies
            // SANS AUCUN TAG AFFILIÉ → 0 commission pour UK, CA, FR, DE, etc.
            if (isOneLinkCountry(targetCountry) && pmPriceCTA.amazonIds['us']) {
                const usTag = pmPriceCTA.amazonIds['us'];
                log(`No ${targetCountry} tag configured - using US tag "${usTag}" (OneLink will convert)`);
                try {
                    const urlObj = new URL(url);
                    urlObj.searchParams.delete('tag');
                    urlObj.searchParams.set('tag', usTag);
                    return urlObj.toString();
                } catch (e) {
                    const separator = url.includes('?') ? '&' : '?';
                    return url + separator + 'tag=' + encodeURIComponent(usTag);
                }
            }
            // PAYS HORS ONELINK SANS TAG LOCAL (AU, IE, BE, IN, AE, etc.)
            // NE PAS appliquer le tag US sur un domaine Amazon non-US !
            // Un tag US sur amazon.com.au/amazon.in/etc. = Amazon IGNORE le tag
            // → 0 commission ET fausse impression que le tracking fonctionne.
            // Mieux vaut laisser l'URL sans tag et logger un WARNING explicite.
            log(`CRITICAL WARNING: No ${targetCountry} Amazon tag configured! ` +
                `Clicks to amazon.${targetCountry.toLowerCase()} will NOT generate commissions. ` +
                `Configure your ${targetCountry} affiliate tag in PM Affiliates settings.`);
            // Ne PAS supprimer un tag existant : il peut être correct (saisi manuellement dans l'URL)
            return url;
        }

        // Ajouter/remplacer le tag
        try {
            const urlObj = new URL(url);
            urlObj.searchParams.delete('tag');
            urlObj.searchParams.set('tag', tag);
            log(`Tag added: ${tag} for ${targetCountry}`);
            return urlObj.toString();
        } catch (e) {
            const separator = url.includes('?') ? '&' : '?';
            return url + separator + 'tag=' + encodeURIComponent(tag);
        }
    }

    // ==========================================
    // 5. MISE À JOUR VISUELS (PRIX & DEVISE)
    // ==========================================

    function updateVisuals(button, country) {
        const symbols = {
            USD: '$', CAD: 'C$', GBP: '£', EUR: '€',
            AUD: 'A$', INR: '₹', AED: 'د.إ'
        };

        // Déterminer la devise selon le pays
        let currency = 'USD';

        if (country === 'CA') {
            currency = 'CAD';
        } else if (['GB', 'UK'].includes(country)) {
            currency = 'GBP';
        } else if (['AU', 'NZ'].includes(country)) {
            currency = 'AUD';
        } else if (country === 'IN') {
            currency = 'INR';
        } else if (['AE', 'SA', 'OM', 'QA', 'KW', 'BH'].includes(country)) {
            currency = 'AED';
        } else if ([
            'FR', 'DE', 'IT', 'ES', 'NL', 'BE', 'IE', 'AT', 'PT', 'FI', 'GR', 'LU',
            'EE', 'LV', 'LT', 'SK', 'SI', 'CY', 'MT',
            'SE', 'PL', 'NO', 'DK', 'CZ', 'HU', 'RO', 'BG', 'HR'
        ].includes(country)) {
            currency = 'EUR';
        }

        // Stocker la devise pour le tracking
        button.dataset.currency = currency.toLowerCase();

        // Récupérer le prix
        let price = null;

        // Prix EUR spécifique par pays
        if (currency === 'EUR') {
            const specificKey = 'priceeur' + country.toLowerCase();
            if (button.dataset[specificKey]) {
                price = button.dataset[specificKey];
            }
        }

        // Prix standard si pas de prix spécifique
        if (!price) {
            const generalKey = 'price' + currency.toLowerCase();
            price = button.dataset[generalKey];
        }

        // Affichage conditionnel
        const shouldDisplayPrices = typeof pmPriceCTA !== 'undefined' &&
            (pmPriceCTA.displayPrices === true || pmPriceCTA.displayPrices === '1' ||
             pmPriceCTA.displayPrices === 1 || pmPriceCTA.displayPrices === 'true');

        if (shouldDisplayPrices && price && parseFloat(price) > 0) {
            const amountEl = button.querySelector('.pm-price-amount');
            const displayEl = button.querySelector('.pm-price-display');
            if (amountEl) {
                amountEl.textContent = (symbols[currency] || '$') + price;
                if (displayEl) {
                    displayEl.style.display = 'inline';
                }
            }
        }
    }

    // ==========================================
    // 6. TRACKING DES CLICS
    // ==========================================

    let currentCountry = 'US';
    let isGeoDetected = false;

    // Event delegation pour capturer tous les clics
    document.addEventListener('click', function(e) {
        const btn = e.target.closest('.pm-price-btn');
        if (!btn) return;

        // Guard: warn if geo detection hasn't completed yet (country may be inaccurate)
        if (!isGeoDetected) {
            log('WARNING: Click before geo detection completed. Country defaults to:', currentCountry);
        }

        // IMPORTANT: Use LOCAL variables for normalization - never mutate module-level currentCountry
        let visitorCountry = currentCountry;
        if (visitorCountry === 'GB') visitorCountry = 'UK';

        const storeType = btn.dataset.store || 'amazon';
        const productName = btn.dataset.product || 'Unknown Product';
        const currency = btn.dataset.currency || 'usd';
        const priceKey = 'price' + currency;
        const price = btn.dataset[priceKey] || 0;

        // Validate URL before allowing navigation - prevent blank page
        const finalLink = btn.href;
        if (!finalLink || finalLink === '#' || finalLink === 'about:blank' || finalLink === window.location.href + '#') {
            log('ERROR: Invalid link detected, preventing click:', finalLink);
            e.preventDefault();
            return;
        }

        // Déterminer le pays cible
        // PRIORITÉ 1: Utiliser le pays résolu à l'init (fiable même pour amzn.to)
        // PRIORITÉ 2: Fallback sur détection depuis URL (pour boutons dynamiques)
        let targetCountry = btn.dataset.pmResolvedCountry || getTargetCountryFromLink(finalLink);

        // Normalize GB -> UK for tracking consistency
        if (targetCountry === 'GB') targetCountry = 'UK';

        log('Click tracked:', {
            product: productName,
            store: storeType,
            visitorCountry: visitorCountry,
            targetCountry: targetCountry,
            price: price,
            link: finalLink.substring(0, 60) + '...',
        });

        // Envoyer le pays du VISITEUR ET le pays de DESTINATION
        trackAffiliateClick(visitorCountry, targetCountry, storeType, productName, price);
    });

    function trackAffiliateClick(visitorCountry, targetCountry, storeType, productName, price) {
        // Google Analytics 4 - avec pays visiteur ET pays destination
        if (typeof gtag !== 'undefined') {
            gtag('event', 'affiliate_click', {
                'product_name': productName,
                'store_type': storeType,
                'visitor_country': visitorCountry,
                'target_country': targetCountry,
                'value': parseFloat(price) || 0,
                'currency': 'USD'
            });
            log('GA4 event sent:', { visitorCountry, targetCountry, productName });
        }

        // Server-side tracking (fiable, pas bloqué par adblockers)
        if (typeof pmPriceCTA !== 'undefined' && pmPriceCTA.ajaxUrl) {
            const trackData = new URLSearchParams({
                action: 'pm_track_click',
                product: productName,
                store: storeType,
                country: visitorCountry,           // Pays du visiteur
                target_country: targetCountry,     // Pays Amazon destination
                price: price
            });

            log('Sending tracking data:', trackData.toString());

            // Utiliser sendBeacon pour fiabilité (ne bloque pas la navigation)
            // CRITIQUE v7.2.2: WordPress AJAX nécessite 'action' dans l'URL pour router
            // vers le bon hook. sendBeacon met les données dans php://input, mais WordPress
            // cherche $_REQUEST['action'] (GET+POST) AVANT d'appeler le handler.
            // Solution: ajouter action en query string pour que WordPress route correctement.
            let sent = false;
            if (navigator.sendBeacon) {
                const blob = new Blob([trackData.toString()], {
                    type: 'application/x-www-form-urlencoded'
                });
                // FIX: Ajouter ?action=pm_track_click à l'URL pour le routing WordPress
                const trackingUrl = pmPriceCTA.ajaxUrl + '?action=pm_track_click';
                sent = navigator.sendBeacon(trackingUrl, blob);
                log('Tracking sent via sendBeacon to:', trackingUrl, 'Result:', sent);
            }

            // Si sendBeacon échoue ou n'existe pas, utiliser fetch
            // FIX: Ajouter ?action= comme pour sendBeacon (WordPress routing)
            if (!sent) {
                fetch(pmPriceCTA.ajaxUrl + '?action=pm_track_click', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: trackData.toString(),
                    keepalive: true
                }).then(res => {
                    log('Tracking response:', res.status);
                    return res.json();
                }).then(data => {
                    log('Tracking data:', data);
                }).catch(err => {
                    log('Tracking error:', err);
                });
            }
        } else {
            log('WARNING: pmPriceCTA not defined or missing ajaxUrl');
        }
    }

    // ==========================================
    // 7. INITIALISATION
    // ==========================================

    async function initSystem() {
        const buttons = document.querySelectorAll('.pm-price-btn');
        if (!buttons.length) {
            log('No CTA buttons found on page');
            return;
        }

        log('Found', buttons.length, 'CTA button(s)');

        // Détection pays (une seule fois)
        if (!isGeoDetected) {
            currentCountry = await detectCountry();
            isGeoDetected = true;
            log('Geo detection complete:', currentCountry);
        }

        buttons.forEach((btn, index) => {
            // Éviter de réinitialiser
            if (btn.dataset.pmProcessed === 'true') return;
            btn.dataset.pmProcessed = 'true';

            log(`Processing button #${index + 1}:`, btn.dataset.product);

            // v7.7: Logger les liens disponibles + tags configurés pour debug
            log('Available links:', {
                us: btn.dataset.linkUs ? btn.dataset.linkUs.substring(0, 40) : 'EMPTY',
                ca: btn.dataset.linkCa ? btn.dataset.linkCa.substring(0, 40) : 'EMPTY',
                uk: btn.dataset.linkUk ? btn.dataset.linkUk.substring(0, 40) : 'EMPTY',
                fr: btn.dataset.linkFr ? btn.dataset.linkFr.substring(0, 40) : 'EMPTY',
                de: btn.dataset.linkDe ? btn.dataset.linkDe.substring(0, 40) : 'EMPTY',
                au: btn.dataset.linkAu ? btn.dataset.linkAu.substring(0, 40) : 'EMPTY',
                ie: btn.dataset.linkIe ? btn.dataset.linkIe.substring(0, 40) : 'EMPTY',
            });
            if (typeof pmPriceCTA !== 'undefined' && pmPriceCTA.amazonIds) {
                log('Configured Amazon tags:', {
                    us: pmPriceCTA.amazonIds.us || 'NOT SET',
                    uk: pmPriceCTA.amazonIds.uk || 'NOT SET',
                    ca: pmPriceCTA.amazonIds.ca || 'NOT SET',
                    fr: pmPriceCTA.amazonIds.fr || 'NOT SET',
                    de: pmPriceCTA.amazonIds.de || 'NOT SET',
                });
            }

            // Save original PHP-rendered href as a guaranteed fallback
            const originalHref = btn.href;

            // Mettre à jour visuels (prix, devise)
            updateVisuals(btn, currentCountry);

            // Obtenir le lien optimal + le pays résolu
            const storeType = btn.dataset.store || 'amazon';
            const optimalResult = getOptimalLink(btn, currentCountry);
            let finalLink = optimalResult.url;
            const resolvedCountry = optimalResult.resolvedCountry;

            // Stocker le pays résolu sur le bouton (crucial pour les liens amzn.to
            // dont on ne peut pas extraire le pays cible depuis l'URL)
            btn.dataset.pmResolvedCountry = resolvedCountry;
            log('Resolved target country:', resolvedCountry, '(visitor:', currentCountry + ')');

            // Ajouter le tag Amazon
            finalLink = addAmazonTag(finalLink, storeType);

            // Mettre à jour le href - only if we have a valid URL
            if (finalLink && finalLink !== btn.href) {
                btn.href = finalLink;
                log('Link updated to:', finalLink.substring(0, 80) + '...', '→ Target:', resolvedCountry);
            } else if (!finalLink || finalLink === '#' || finalLink === '') {
                // SAFETY: If link resolution failed, restore PHP fallback
                btn.href = originalHref;
                log('WARNING: Link resolution failed, restored original:', originalHref.substring(0, 80));
            } else {
                log('Link unchanged:', btn.href.substring(0, 80) + '...');
            }
        });

        log('Initialization complete');
    }

    // ==========================================
    // 8. SUPPORT CONTENU DYNAMIQUE (AJAX)
    // ==========================================

    function watchForNewButtons() {
        const observer = new MutationObserver((mutations) => {
            let hasNewButtons = false;

            mutations.forEach(mutation => {
                mutation.addedNodes.forEach(node => {
                    if (node.nodeType === 1) {
                        if (node.classList && node.classList.contains('pm-price-btn')) {
                            hasNewButtons = true;
                        } else if (node.querySelectorAll) {
                            const newBtns = node.querySelectorAll('.pm-price-btn:not([data-pm-processed="true"])');
                            if (newBtns.length > 0) hasNewButtons = true;
                        }
                    }
                });
            });

            if (hasNewButtons) {
                log('New buttons detected, reinitializing...');
                initSystem();
            }
        });

        observer.observe(document.body, {
            childList: true,
            subtree: true
        });
    }

    // ==========================================
    // 9. GROUPEMENT CÔTE-À-CÔTE DES BOUTONS ADJACENTS
    // ==========================================

    function groupAdjacentCTAs() {
        const ctaSelector = '.pm-cta-wrapper, .pm-price-group';
        const allCtas = document.querySelectorAll(ctaSelector);
        if (allCtas.length < 2) return;

        const processed = new Set();

        allCtas.forEach(function(cta) {
            if (processed.has(cta) || cta.closest('.pm-cta-inline-group')) return;

            var group = [cta];
            processed.add(cta);

            var nextCta = findNextAdjacentCTA(cta, ctaSelector);
            while (nextCta && !processed.has(nextCta)) {
                group.push(nextCta);
                processed.add(nextCta);
                nextCta = findNextAdjacentCTA(nextCta, ctaSelector);
            }

            if (group.length >= 2) {
                wrapCTAGroup(group);
            }
        });
    }

    function findNextAdjacentCTA(el, ctaSelector) {
        // Determine the "block" element: the CTA itself, or its wp-block-html parent
        var block = getBlockContainer(el);
        var next = getNextMeaningfulSibling(block);
        if (!next) return null;

        // Direct CTA sibling
        if (next.matches && next.matches(ctaSelector)) return next;

        // CTA inside a block wrapper (wp-block-html)
        if (next.querySelector) {
            var inner = next.querySelector(ctaSelector);
            if (inner) return inner;
        }

        return null;
    }

    function getBlockContainer(cta) {
        var parent = cta.parentElement;
        if (parent && parent.classList && parent.classList.contains('wp-block-html')) {
            return parent;
        }
        return cta;
    }

    function getNextMeaningfulSibling(el) {
        var node = el.nextSibling;
        while (node) {
            if (node.nodeType === 1) return node;
            if (node.nodeType === 3 && node.textContent.trim() !== '') return null;
            node = node.nextSibling;
        }
        return null;
    }

    function wrapCTAGroup(group) {
        var wrapper = document.createElement('div');
        wrapper.className = 'pm-cta-inline-group';

        var firstBlock = getBlockContainer(group[0]);
        firstBlock.parentNode.insertBefore(wrapper, firstBlock);

        group.forEach(function(cta) {
            wrapper.appendChild(getBlockContainer(cta));
        });

        // Consolidate Amazon disclaimers: keep one, remove duplicates, position below buttons
        var disclaimers = wrapper.querySelectorAll('.pm-affiliate-disclaimer');
        if (disclaimers.length > 1) {
            var kept = disclaimers[0];
            for (var i = 1; i < disclaimers.length; i++) {
                disclaimers[i].remove();
            }
            // Move the single disclaimer as direct child of the group (full width below buttons)
            wrapper.appendChild(kept);
        } else if (disclaimers.length === 1) {
            wrapper.appendChild(disclaimers[0]);
        }

        log('Grouped ' + group.length + ' adjacent CTA buttons side-by-side');
    }

    // ==========================================
    // 10. LANCEMENT
    // ==========================================

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', function() {
            groupAdjacentCTAs();
            initSystem();
            watchForNewButtons();
        });
    } else {
        groupAdjacentCTAs();
        initSystem();
        watchForNewButtons();
    }

    // Exposer pour debug manuel
    window.pmCTADebug = {
        detectCountry,
        getOptimalLink,
        currentCountry: () => currentCountry,
        reinit: initSystem
    };

})();