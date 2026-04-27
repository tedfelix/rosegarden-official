<?php
/**
 * PianoMode Spotify Embed Shortcode
 * Compact, designé, insérable partout via shortcode.
 *
 * @package PianoMode
 */

if (!defined('ABSPATH')) {
    exit;
}

/**
 * PIANOMODE SPOTIFY EMBED PLAYER
 *
 * Usage:
 *   [pm_spotify url="https://open.spotify.com/track/xxx"]
 *   [pm_spotify url="https://open.spotify.com/album/xxx"]
 *   [pm_spotify url="https://open.spotify.com/playlist/xxx"]
 *   [pm_spotify url="https://open.spotify.com/track/xxx" size="large"]
 *   [pm_spotify url="https://open.spotify.com/album/xxx" size="compact"]
 *   [pm_spotify url="..." theme="light"]  (light | dark, défaut: dark)
 *   [pm_spotify url="..." title="Mon titre personnalisé"]
 *
 * Auto-détecte track/album/playlist depuis l'URL.
 * Tailles par défaut : track → compact (80px), album/playlist → large (352px).
 */
function pm_spotify_player_shortcode($atts) {

    $atts = shortcode_atts(array(
        'url'   => '',
        'size'  => '',      // compact | large  (auto si vide)
        'theme' => 'dark',  // dark | light
        'title' => '',      // titre affiché au-dessus (optionnel)
    ), $atts, 'pm_spotify');

    $url = esc_url(trim($atts['url']));
    if (!$url || strpos($url, 'open.spotify.com') === false) {
        return '<!-- pm_spotify: URL Spotify manquante ou invalide -->';
    }

    // --- Extraire type et ID depuis l'URL ---
    if (preg_match('#open\.spotify\.com/(?:intl-[a-z]{2}/)?(track|album|playlist)/([a-zA-Z0-9]+)#', $url, $m)) {
        $type = $m[1];
        $id   = $m[2];
    } else {
        return '<!-- pm_spotify: URL Spotify non reconnue -->';
    }

    // --- Taille ---
    $size = strtolower(trim($atts['size']));
    if ($size === '') {
        $size = ($type === 'track') ? 'compact' : 'large';
    }
    $height = ($size === 'compact') ? 80 : 352;

    // --- Theme ---
    $theme_param = ($atts['theme'] === 'light') ? 1 : 0;

    // --- Embed URL (format officiel Spotify) ---
    $embed_url = "https://open.spotify.com/embed/{$type}/{$id}?utm_source=generator&theme={$theme_param}";

    // --- Titre optionnel ---
    $title_html = '';
    $title_text = sanitize_text_field($atts['title']);
    if ($title_text !== '') {
        $title_html = '<div class="pm-sp-title">' . esc_html($title_text) . '</div>';
    }

    // --- Classe CSS selon le contexte ---
    $wrapper_class = 'pm-sp pm-sp--' . $type . ' pm-sp--' . $size;

    // CSS inline uniquement au premier appel
    static $css_printed = false;
    $css = '';
    if (!$css_printed) {
        $css_printed = true;
        $css = '<style>
/* ── PianoMode Spotify Player ───────────────────────── */
.pm-sp{
    position:relative;
    width:100%;
    border-radius:var(--pianomode-radius-md,12px);
    overflow:hidden;
    background:#0d0d0d;
    box-shadow:0 4px 24px rgba(0,0,0,.12);
    transition:var(--pianomode-transition-smooth,all .3s cubic-bezier(.4,0,.2,1));
    border:1px solid rgba(255,255,255,.06);
}
.pm-sp:hover{
    box-shadow:0 8px 36px rgba(0,0,0,.18);
    border-color:rgba(29,185,84,.25);
}
.pm-sp::before{
    content:"";
    position:absolute;top:0;left:0;right:0;
    height:2px;
    background:linear-gradient(90deg,var(--pianomode-primary-gold,#D7BF81),#1DB954);
    opacity:0;
    transition:opacity .3s ease;
    z-index:2;
}
.pm-sp:hover::before{opacity:1}
/* Title bar */
.pm-sp-title{
    padding:10px 16px 0;
    font-family:var(--pianomode-font-family,"Montserrat",sans-serif);
    font-size:.8rem;
    font-weight:600;
    letter-spacing:.04em;
    text-transform:uppercase;
    color:rgba(255,255,255,.55);
    display:flex;
    align-items:center;
    gap:8px;
}
.pm-sp-title::before{
    content:"";
    display:inline-block;
    width:8px;height:8px;
    background:#1DB954;
    border-radius:50%;
    flex-shrink:0;
    box-shadow:0 0 6px rgba(29,185,84,.5);
}
/* iframe */
.pm-sp iframe{
    display:block;
    width:100%!important;
    border:none!important;
    border-radius:0!important;
}
/* Compact (track bar) */
.pm-sp--compact{border-radius:var(--pianomode-radius-sm,8px)}
.pm-sp--compact .pm-sp-title{padding:8px 14px 0;font-size:.7rem}
/* Light variant */
.pm-sp--light{background:#f8f9fa;border-color:rgba(0,0,0,.08)}
.pm-sp--light .pm-sp-title{color:rgba(0,0,0,.5)}
.pm-sp--light:hover{border-color:rgba(29,185,84,.3);box-shadow:0 8px 36px rgba(0,0,0,.1)}
/* Responsive */
@media(max-width:600px){
    .pm-sp-title{font-size:.7rem;padding:8px 12px 0}
    .pm-sp--compact .pm-sp-title{font-size:.65rem}
}
</style>';
    }

    // --- Light mode class ---
    if ($atts['theme'] === 'light') {
        $wrapper_class .= ' pm-sp--light';
    }

    return $css . '<div class="' . esc_attr($wrapper_class) . '">'
        . $title_html
        . '<iframe src="' . esc_url($embed_url) . '" '
        . 'height="' . (int) $height . '" '
        . 'frameborder="0" allowfullscreen '
        . 'allow="autoplay; clipboard-write; encrypted-media; fullscreen; picture-in-picture" '
        . 'loading="lazy" '
        . 'style="width:100%">'
        . '</iframe></div>';
}
add_shortcode('pm_spotify', 'pm_spotify_player_shortcode');