<?php
/**
 * PianoMode Premium Badge Renderer
 * Centralized SVG badge rendering with 3D-like premium design
 * All icons are perfectly centered in 80x80 viewBox (center: 40,40)
 *
 * @package Blocksy-child
 * @version 2.0.0
 */

if (!defined('ABSPATH')) exit;

/**
 * Get tier configuration (colors, gradients, effects)
 */
function pianomode_badge_tier_config($tier) {
    $tiers = array(
        'bronze' => array(
            'primary'    => '#D4853A',
            'secondary'  => '#8B4513',
            'shine'      => '#F0B87C',
            'dark'       => '#4A2810',
            'glow'       => 'rgba(212,133,58,0.4)',
            'label'      => 'Bronze',
            'ring'       => '#B07030',
        ),
        'silver' => array(
            'primary'    => '#C8CDD5',
            'secondary'  => '#8A9099',
            'shine'      => '#F0F2F5',
            'dark'       => '#3A3E45',
            'glow'       => 'rgba(200,205,213,0.5)',
            'label'      => 'Silver',
            'ring'       => '#A0A8B5',
        ),
        'gold' => array(
            'primary'    => '#FFD700',
            'secondary'  => '#C89B00',
            'shine'      => '#FFF0A0',
            'dark'       => '#7A5C00',
            'glow'       => 'rgba(255,215,0,0.55)',
            'label'      => 'Gold',
            'ring'       => '#E8BF00',
        ),
        'diamond' => array(
            'primary'    => '#B0E8FF',
            'secondary'  => '#0099BB',
            'shine'      => '#E5F8FF',
            'dark'       => '#004D60',
            'glow'       => 'rgba(0,180,220,0.5)',
            'label'      => 'Diamond',
            'ring'       => '#60D0EE',
        ),
        'legendary' => array(
            'primary'    => '#D4A840',
            'secondary'  => '#1A1200',
            'shine'      => '#FFE680',
            'dark'       => '#0E0E0E',
            'glow'       => 'rgba(212,168,64,0.65)',
            'label'      => 'Legendary',
            'ring'       => '#E0B840',
        ),
    );
    return $tiers[$tier] ?? $tiers['bronze'];
}

/**
 * Get the inner icon SVG for a given icon type.
 * All icons are drawn centered at (0,0) in a ~40x40 space, then translated to (40,40) by the badge renderer.
 */
function pianomode_badge_icon_path($icon) {
    $icons = array(
        // Star - clean 5-pointed star, centered at 0,0
        'star' => '<polygon points="0,-16 4.9,-5.2 16,-5.2 7.8,1.8 10.5,13 0,6.5 -10.5,13 -7.8,1.8 -16,-5.2 -4.9,-5.2" fill="currentColor"/>',

        // Flame - asymmetric fire with jagged flickering edges and forked tip, centered at 0,0
        'flame' => '<path d="M0,-18 C1,-14 -1,-10 -2,-6 C-3,-2 -5,2 -7,6 C-8,8 -9,11 -8.5,14 C-8,17 -5.5,19 -2,19 C0,19 2,19 4,18 C7,17 9,14 8.5,11 C8,8 7,5 5.5,2 C4,-1 2,-5 1,-9 C0.5,-11 0,-15 0,-18z" fill="currentColor"/><path d="M-3,-16 C-4,-12 -6,-7 -8,-3 C-10,2 -11,6 -10.5,10 C-10,12 -9,13.5 -7.5,12 C-6.5,11 -7,8 -6.5,5 C-6,2 -5,-2 -3,-7 C-2,-10 -3,-13 -3,-16z" fill="currentColor" opacity="0.45"/><path d="M3,-14 C5,-9 7,-4 8,1 C9,5 9.5,8 9,11 C8.5,13 7.5,13.5 6.5,12 C6,10.5 6.5,8 6,5 C5.5,2 4,-2 3,-6 C2.5,-9 3,-12 3,-14z" fill="currentColor" opacity="0.4"/><path d="M-1,-11 C-2,-7 -3,-3 -2,1 C-1,4 0,6 1,4 C1.5,2 0,-2 -0.5,-5 C-1,-8 -1,-10 -1,-11z" fill="currentColor" opacity="0.3"/><path d="M2,-8 C3,-5 4,-1 3.5,2 C3,4 2,5 1.5,3 C1,1 1.5,-3 2,-6 C2,-7 2,-8 2,-8z" fill="currentColor" opacity="0.25"/>',

        // Fire (intense multi-tongue flame) - centered at 0,0
        'fire' => '<path d="M0,-19 C1,-14 -1,-9 -3,-4 C-5,1 -7,5 -9,9 C-10,12 -10.5,15 -9,17 C-7,19 -3.5,20 0,20 C3.5,20 7,19 9,17 C10.5,15 10,12 9,9 C7,5 5,1 3,-4 C1,-9 -1,-14 0,-19z" fill="currentColor"/><path d="M-5,-14 C-7,-9 -9,-4 -11,1 C-12.5,5 -13,9 -12,12 C-11,14 -9.5,14 -8.5,12 C-7.5,9 -8,6 -7,2 C-6,-2 -5,-7 -5,-14z" fill="currentColor" opacity="0.4"/><path d="M5,-16 C7,-11 10,-5 11,0 C12,4 12.5,8 11.5,11 C10.5,13 9,13 8.5,11 C8,9 8.5,5 8,2 C7,-2 6,-8 5,-16z" fill="currentColor" opacity="0.35"/><path d="M-2,-13 C-3,-9 -4,-4 -3,0 C-2,3 -1,5 0,3 C0.5,1 0,-3 -0.5,-6 C-1,-9 -1.5,-11 -2,-13z" fill="currentColor" opacity="0.3"/><path d="M2,-11 C3,-7 4,-2 3,2 C2.5,5 1.5,6 1,4 C0.5,2 1.5,-2 2,-5 C2,-8 2,-10 2,-11z" fill="currentColor" opacity="0.25"/><path d="M0,20 C-3,20 -5.5,18 -5.5,15.5 C-5.5,12 -3,8 0,4 C3,8 5.5,12 5.5,15.5 C5.5,18 3,20 0,20z" fill="currentColor" opacity="0.2"/>',

        // Play button - centered at 0,0
        'play' => '<path d="M-8,-14L14,0L-8,14z" fill="currentColor"/>',

        // Clock - centered at 0,0
        'clock' => '<circle cx="0" cy="0" r="15" fill="none" stroke="currentColor" stroke-width="2.5"/><polyline points="0,-8 0,1 7,4" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"/>',

        // Double music notes - clean beamed pair, centered at 0,0
        'music' => '<line x1="-5" y1="8" x2="-5" y2="-10" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/><line x1="9" y1="3" x2="9" y2="-15" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/><path d="M-5,-10 L9,-15" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round"/><ellipse cx="-8" cy="9" rx="5" ry="3.5" transform="rotate(-15,-8,9)" fill="currentColor"/><ellipse cx="6" cy="4" rx="5" ry="3.5" transform="rotate(-15,6,4)" fill="currentColor"/>',

        // Single note - clean eighth note, centered at 0,0
        'note' => '<line x1="3" y1="8" x2="3" y2="-14" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/><path d="M3,-14 C8,-12 12,-8 8,-4" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/><ellipse cx="0" cy="9" rx="5" ry="3.5" transform="rotate(-15,0,9)" fill="currentColor"/>',

        // Open book - centered at 0,0, shifted up 3px for visual centering
        'book_open' => '<g transform="translate(0,-3)"><path d="M0,-6L0,12" stroke="currentColor" stroke-width="1.5"/><path d="M0,-6C-2,-8-5,-10-14,-10V8c9,0 12,2 14,4" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"/><path d="M0,-6C2,-8,5,-10,14,-10V8c-9,0-12,2-14,4" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"/><line x1="-10" y1="-5" x2="-3" y2="-5" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="-10" y1="-1" x2="-3" y2="-1" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="-10" y1="3" x2="-4" y2="3" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="3" y1="-5" x2="10" y2="-5" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="3" y1="-1" x2="10" y2="-1" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="4" y1="3" x2="10" y2="3" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><circle cx="-7" cy="-8" r="1" fill="currentColor" opacity="0.4"/><circle cx="7" cy="-8" r="1" fill="currentColor" opacity="0.4"/></g>',

        // Closed book (for aliases) - shifted up 3px
        'book' => '<g transform="translate(0,-3)"><path d="M0,-6L0,12" stroke="currentColor" stroke-width="1.5"/><path d="M0,-6C-2,-8-5,-10-14,-10V8c9,0 12,2 14,4" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"/><path d="M0,-6C2,-8,5,-10,14,-10V8c-9,0-12,2-14,4" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"/><line x1="-10" y1="-5" x2="-3" y2="-5" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="-10" y1="-1" x2="-3" y2="-1" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="-10" y1="3" x2="-4" y2="3" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="3" y1="-5" x2="10" y2="-5" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="3" y1="-1" x2="10" y2="-1" stroke="currentColor" stroke-width="1.2" opacity="0.5"/><line x1="4" y1="3" x2="10" y2="3" stroke="currentColor" stroke-width="1.2" opacity="0.5"/></g>',

        // Library (book stack with shelf) - centered at 0,0, shifted up 2px
        'library' => '<g transform="translate(0,-2)"><rect x="-13" y="-8" width="4.5" height="18" rx="1" fill="currentColor" opacity="0.9"/><rect x="-7" y="-12" width="4.5" height="22" rx="1" fill="currentColor" opacity="0.75"/><rect x="-1" y="-6" width="4.5" height="16" rx="1" fill="currentColor" opacity="0.9"/><rect x="5" y="-10" width="4.5" height="20" rx="1" fill="currentColor" opacity="0.75"/><rect x="11" y="-7" width="4" height="17" rx="1" fill="currentColor" opacity="0.85"/><line x1="-15" y1="10" x2="16" y2="10" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" opacity="0.6"/></g>',

        // Trophy - elegant cup with curved handles and sturdy base, centered at 0,0
        'trophy' => '<path d="M-9,-14 L-9,-4 C-9,3 -5,8 0,10 C5,8 9,3 9,-4 L9,-14z" fill="currentColor"/><rect x="-10" y="-15.5" width="20" height="2.5" rx="1.2" fill="currentColor" opacity="0.8"/><path d="M-9,-11 C-9,-11 -14,-11 -14.5,-6 C-15,-1 -11,3 -8,3" fill="none" stroke="currentColor" stroke-width="2.8" stroke-linecap="round"/><path d="M9,-11 C9,-11 14,-11 14.5,-6 C15,-1 11,3 8,3" fill="none" stroke="currentColor" stroke-width="2.8" stroke-linecap="round"/><rect x="-1.5" y="10" width="3" height="4.5" rx="0.5" fill="currentColor"/><rect x="-7" y="14.5" width="14" height="3" rx="1.5" fill="currentColor"/><path d="M-5,-10 L-2,-4 L0,-8 L2,-4 L5,-10" fill="none" stroke="currentColor" stroke-width="1" opacity="0.2" stroke-linecap="round"/>',

        // Medal - circular medal with ribbon, centered at 0,0
        'medal' => '<circle cx="0" cy="-2" r="10" fill="currentColor" opacity="0.9"/><circle cx="0" cy="-2" r="7" fill="none" stroke="currentColor" stroke-width="1.5" opacity="0.4"/><polygon points="0,-6 1.5,-3 5,-2.5 2.5,0 3,3.5 0,2 -3,3.5 -2.5,0 -5,-2.5 -1.5,-3" fill="currentColor" opacity="0.35"/><path d="M-6,7 L-10,18" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"/><path d="M6,7 L10,18" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"/><path d="M-10,18 L-4,14 L0,18 L4,14 L10,18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>',

        // Crown - clean, well-proportioned, centered at 0,0
        'crown' => '<path d="M-14,10 L-14,-2 L-7,4 L0,-14 L7,4 L14,-2 L14,10z" fill="currentColor"/><circle cx="-14" cy="-4" r="2.5" fill="currentColor"/><circle cx="0" cy="-16" r="2.5" fill="currentColor"/><circle cx="14" cy="-4" r="2.5" fill="currentColor"/><rect x="-14" y="10" width="28" height="4" rx="2" fill="currentColor" opacity="0.7"/>',

        // Treble Clef (G Clef) - uses Unicode 𝄞 rendered as text, shifted up for visual centering
        'treble' => '<text x="0" y="-2" text-anchor="middle" dominant-baseline="central" fill="currentColor" font-size="44" font-family="&quot;Times New Roman&quot;,Georgia,&quot;Noto Music&quot;,serif">&#x1D11E;</text>',

        // Bass Clef (F Clef) - uses Unicode 𝄢 rendered as text, centered precisely
        'bass' => '<text x="0" y="4" text-anchor="middle" dominant-baseline="central" fill="currentColor" font-size="44" font-family="&quot;Times New Roman&quot;,Georgia,&quot;Noto Music&quot;,serif">&#x1D122;</text>',

        // Piano keyboard - 7 white keys (C-B) with 5 black keys symmetrically placed, centered at 0,0
        // White keys: each ~4.1px wide spanning 28.7px total, centered
        'piano' => '<rect x="-14.5" y="-12" width="29" height="24" rx="1.5" fill="currentColor" opacity="0.12"/><rect x="-14" y="-11.5" width="3.9" height="23" rx="0.4" fill="none" stroke="currentColor" stroke-width="0.7"/><rect x="-10.1" y="-11.5" width="3.9" height="23" rx="0.4" fill="none" stroke="currentColor" stroke-width="0.7"/><rect x="-6.2" y="-11.5" width="3.9" height="23" rx="0.4" fill="none" stroke="currentColor" stroke-width="0.7"/><rect x="-2.3" y="-11.5" width="4.1" height="23" rx="0.4" fill="none" stroke="currentColor" stroke-width="0.7"/><rect x="1.8" y="-11.5" width="4.1" height="23" rx="0.4" fill="none" stroke="currentColor" stroke-width="0.7"/><rect x="5.9" y="-11.5" width="4.1" height="23" rx="0.4" fill="none" stroke="currentColor" stroke-width="0.7"/><rect x="10" y="-11.5" width="4.5" height="23" rx="0.4" fill="none" stroke="currentColor" stroke-width="0.7"/><rect x="-11.3" y="-11.5" width="2.5" height="14" rx="0.4" fill="currentColor"/><rect x="-7.4" y="-11.5" width="2.5" height="14" rx="0.4" fill="currentColor"/><rect x="0.3" y="-11.5" width="2.5" height="14" rx="0.4" fill="currentColor"/><rect x="4.4" y="-11.5" width="2.5" height="14" rx="0.4" fill="currentColor"/><rect x="8.5" y="-11.5" width="2.5" height="14" rx="0.4" fill="currentColor"/>',

        // Treble clef sketch (dashed style) - for level_5, shifted up to match solid treble
        'treble_sketch' => '<text x="0" y="-2" text-anchor="middle" dominant-baseline="central" fill="none" stroke="currentColor" stroke-width="0.8" stroke-dasharray="3,2" font-size="44" font-family="&quot;Times New Roman&quot;,Georgia,&quot;Noto Music&quot;,serif">&#x1D11E;</text>',

        // PianoMode logo - Futuristic legendary badge with hexagonal frame, orbital rings, and energy core
        'pianomode' => '
            <!-- Outer hexagonal energy frame -->
            <polygon points="0,-19 16.5,-9.5 16.5,9.5 0,19 -16.5,9.5 -16.5,-9.5" fill="none" stroke="currentColor" stroke-width="0.8" opacity="0.35"/>
            <polygon points="0,-16 13.8,-8 13.8,8 0,16 -13.8,8 -13.8,-8" fill="none" stroke="currentColor" stroke-width="0.4" opacity="0.2"/>
            <!-- Corner accent nodes -->
            <circle cx="0" cy="-19" r="1.2" fill="currentColor" opacity="0.5"/>
            <circle cx="16.5" cy="-9.5" r="1.2" fill="currentColor" opacity="0.4"/>
            <circle cx="16.5" cy="9.5" r="1.2" fill="currentColor" opacity="0.4"/>
            <circle cx="0" cy="19" r="1.2" fill="currentColor" opacity="0.5"/>
            <circle cx="-16.5" cy="9.5" r="1.2" fill="currentColor" opacity="0.4"/>
            <circle cx="-16.5" cy="-9.5" r="1.2" fill="currentColor" opacity="0.4"/>
            <!-- Orbital rings -->
            <ellipse cx="0" cy="0" rx="14" ry="5" fill="none" stroke="currentColor" stroke-width="0.5" opacity="0.2" transform="rotate(-30)"/>
            <ellipse cx="0" cy="0" rx="14" ry="5" fill="none" stroke="currentColor" stroke-width="0.5" opacity="0.2" transform="rotate(30)"/>
            <!-- Inner energy diamond -->
            <polygon points="0,-10 7,0 0,10 -7,0" fill="none" stroke="currentColor" stroke-width="0.6" opacity="0.25"/>
            <!-- Radial accent lines -->
            <line x1="0" y1="-12" x2="0" y2="-16" stroke="currentColor" stroke-width="0.4" opacity="0.2"/>
            <line x1="10.4" y1="-6" x2="13.8" y2="-8" stroke="currentColor" stroke-width="0.4" opacity="0.2"/>
            <line x1="10.4" y1="6" x2="13.8" y2="8" stroke="currentColor" stroke-width="0.4" opacity="0.2"/>
            <line x1="0" y1="12" x2="0" y2="16" stroke="currentColor" stroke-width="0.4" opacity="0.2"/>
            <line x1="-10.4" y1="6" x2="-13.8" y2="8" stroke="currentColor" stroke-width="0.4" opacity="0.2"/>
            <line x1="-10.4" y1="-6" x2="-13.8" y2="-8" stroke="currentColor" stroke-width="0.4" opacity="0.2"/>
            <!-- Crown above PM -->
            <path d="M-7,-8 L-7,-12 L-3.5,-10 L0,-14 L3.5,-10 L7,-12 L7,-8" fill="currentColor" opacity="0.45"/>
            <circle cx="0" cy="-14.5" r="1" fill="currentColor" opacity="0.6"/>
            <circle cx="-7" cy="-12.5" r="0.8" fill="currentColor" opacity="0.4"/>
            <circle cx="7" cy="-12.5" r="0.8" fill="currentColor" opacity="0.4"/>
            <!-- PM text - bold center -->
            <text x="0" y="4" text-anchor="middle" fill="currentColor" font-size="18" font-weight="800" font-family="system-ui,-apple-system,&quot;Segoe UI&quot;,sans-serif" letter-spacing="1">PM</text>
            <!-- Laurel branches (elegant, visible) -->
            <path d="M-9,10 C-10,8 -12,6 -12,3 C-12,0 -10,-2 -9,-3" fill="none" stroke="currentColor" stroke-width="0.7" opacity="0.4" stroke-linecap="round"/>
            <path d="M-10,8 C-12,7 -13,5 -12,3" fill="currentColor" opacity="0.25"/>
            <path d="M-11,5 C-13,3 -13,1 -12,0" fill="currentColor" opacity="0.25"/>
            <path d="M-11,2 C-12,0 -12,-2 -10,-3" fill="currentColor" opacity="0.2"/>
            <path d="M9,10 C10,8 12,6 12,3 C12,0 10,-2 9,-3" fill="none" stroke="currentColor" stroke-width="0.7" opacity="0.4" stroke-linecap="round"/>
            <path d="M10,8 C12,7 13,5 12,3" fill="currentColor" opacity="0.25"/>
            <path d="M11,5 C13,3 13,1 12,0" fill="currentColor" opacity="0.25"/>
            <path d="M11,2 C12,0 12,-2 10,-3" fill="currentColor" opacity="0.2"/>
            <!-- Bottom star accent -->
            <polygon points="0,14 1.2,16 3.5,16 1.5,17.5 2.2,19.5 0,18 -2.2,19.5 -1.5,17.5 -3.5,16 -1.2,16" fill="currentColor" opacity="0.3"/>
        ',

        // Level 20 Legend badge - crown with "20" and laurel
        'level_20' => '
            <!-- Laurel wreath circle -->
            <path d="M-14,12 C-16,6 -17,0 -16,-6 C-15,-10 -13,-13 -10,-14" fill="none" stroke="currentColor" stroke-width="0.8" opacity="0.3"/>
            <path d="M-14,10 C-17,6 -16,2 -13,1" fill="currentColor" opacity="0.18"/>
            <path d="M-16,4 C-18,0 -17,-4 -14,-5" fill="currentColor" opacity="0.18"/>
            <path d="M-16,-2 C-17,-6 -15,-9 -12,-9" fill="currentColor" opacity="0.18"/>
            <path d="M14,12 C16,6 17,0 16,-6 C15,-10 13,-13 10,-14" fill="none" stroke="currentColor" stroke-width="0.8" opacity="0.3"/>
            <path d="M14,10 C17,6 16,2 13,1" fill="currentColor" opacity="0.18"/>
            <path d="M16,4 C18,0 17,-4 14,-5" fill="currentColor" opacity="0.18"/>
            <path d="M16,-2 C17,-6 15,-9 12,-9" fill="currentColor" opacity="0.18"/>
            <!-- Crown on top -->
            <path d="M-8,-6 L-8,-12 L-4,-9 L0,-16 L4,-9 L8,-12 L8,-6z" fill="currentColor" opacity="0.7"/>
            <circle cx="-8" cy="-13" r="1.5" fill="currentColor" opacity="0.5"/>
            <circle cx="0" cy="-17" r="1.8" fill="currentColor" opacity="0.6"/>
            <circle cx="8" cy="-13" r="1.5" fill="currentColor" opacity="0.5"/>
            <!-- Number 20 -->
            <text x="0" y="8" text-anchor="middle" fill="currentColor" font-size="20" font-weight="800" font-family="system-ui,-apple-system,&quot;Segoe UI&quot;,sans-serif">20</text>
        ',
    );

    // Handle aliases
    $path = $icons[$icon] ?? $icons['star'];
    return $path;
}

/**
 * Render a premium SVG badge
 *
 * @param string $achievement_id The achievement identifier
 * @param string $tier bronze|silver|gold|diamond|legendary
 * @param string $icon The icon type
 * @param int $size Badge size in pixels
 * @param bool $earned Whether the user has earned this badge
 * @return string Inline SVG markup
 */
function pianomode_render_badge_svg($achievement_id, $tier, $icon, $size = 80, $earned = true) {
    $t = pianomode_badge_tier_config($tier);
    $icon_svg = pianomode_badge_icon_path($icon);
    $uid = 'pm-badge-' . $achievement_id . '-' . mt_rand(1000, 9999);

    $opacity = $earned ? '1' : '0.35';
    $filter_grayscale = $earned ? '' : ' filter: grayscale(80%);';

    // Legendary gets special animated glow
    $legendary_extras = '';
    if ($tier === 'legendary') {
        $legendary_extras = '<animate attributeName="opacity" values="0.4;0.9;0.4" dur="2.5s" repeatCount="indefinite"/>';
    }

    $svg = '<svg xmlns="http://www.w3.org/2000/svg" width="' . $size . '" height="' . $size . '" viewBox="0 0 80 80" style="opacity:' . $opacity . ';' . $filter_grayscale . '" class="pm-premium-badge pm-badge-' . esc_attr($tier) . '">';

    // Defs - Enhanced gradients for premium 3D look
    $svg .= '<defs>';
    // Background gradient: more dramatic metallic sweep
    $svg .= '<linearGradient id="' . $uid . '-bg" x1="0" y1="0" x2="0.4" y2="1"><stop offset="0%" stop-color="' . $t['shine'] . '"/><stop offset="30%" stop-color="' . $t['primary'] . '"/><stop offset="70%" stop-color="' . $t['ring'] . '"/><stop offset="100%" stop-color="' . $t['dark'] . '"/></linearGradient>';
    // Ring gradient: richer metallic bevel
    $svg .= '<linearGradient id="' . $uid . '-ring" x1="0.1" y1="0" x2="0.9" y2="1"><stop offset="0%" stop-color="' . $t['shine'] . '"/><stop offset="25%" stop-color="' . $t['primary'] . '"/><stop offset="75%" stop-color="' . $t['ring'] . '"/><stop offset="100%" stop-color="' . $t['dark'] . '"/></linearGradient>';
    // Inner dark radial: deeper, richer dark center
    $svg .= '<radialGradient id="' . $uid . '-inner" cx="0.4" cy="0.35" r="0.65"><stop offset="0%" stop-color="#1e1e3a"/><stop offset="60%" stop-color="#111122"/><stop offset="100%" stop-color="#080810"/></radialGradient>';
    // Specular highlight: stronger shine
    $svg .= '<linearGradient id="' . $uid . '-hl" x1="0.2" y1="0" x2="0.8" y2="0.6"><stop offset="0%" stop-color="white" stop-opacity="0.45"/><stop offset="50%" stop-color="white" stop-opacity="0.1"/><stop offset="100%" stop-color="white" stop-opacity="0"/></linearGradient>';
    // Bottom reflection
    $svg .= '<linearGradient id="' . $uid . '-ref" x1="0.5" y1="0.7" x2="0.5" y2="1"><stop offset="0%" stop-color="white" stop-opacity="0"/><stop offset="100%" stop-color="' . $t['primary'] . '" stop-opacity="0.12"/></linearGradient>';
    // Drop shadow + inner glow filter
    $svg .= '<filter id="' . $uid . '-sh" x="-25%" y="-25%" width="150%" height="150%"><feDropShadow dx="0" dy="2" stdDeviation="3.5" flood-color="#000" flood-opacity="0.5"/></filter>';
    // Icon glow filter
    $svg .= '<filter id="' . $uid . '-ig" x="-30%" y="-30%" width="160%" height="160%"><feGaussianBlur in="SourceGraphic" stdDeviation="1.5" result="blur"/><feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge></filter>';
    $svg .= '</defs>';

    // === BADGE LAYERS (Premium 3D Design) ===

    // 0. Ambient shadow (all tiers)
    $svg .= '<circle cx="40" cy="42" r="35" fill="#000" opacity="0.15"/>';

    // 1. Outer glow (silver+)
    if ($tier === 'diamond' || $tier === 'legendary') {
        $svg .= '<circle cx="40" cy="40" r="39" fill="' . $t['glow'] . '" opacity="0.35">' . $legendary_extras . '</circle>';
    } elseif ($tier === 'gold') {
        $svg .= '<circle cx="40" cy="40" r="38" fill="' . $t['glow'] . '" opacity="0.15"/>';
    }

    // 2. Outer ring with bevel shadow
    $svg .= '<circle cx="40" cy="40" r="37" fill="url(#' . $uid . '-ring)" filter="url(#' . $uid . '-sh)"/>';

    // 3. Thin bright edge (metallic rim highlight)
    $svg .= '<circle cx="40" cy="40" r="37" fill="none" stroke="' . $t['shine'] . '" stroke-width="0.6" opacity="0.5"/>';

    // 4. Inner metallic ring
    $svg .= '<circle cx="40" cy="40" r="34" fill="url(#' . $uid . '-bg)"/>';

    // 5. Engraved groove between ring and center
    $svg .= '<circle cx="40" cy="40" r="30.5" fill="none" stroke="' . $t['dark'] . '" stroke-width="1" opacity="0.5"/>';
    $svg .= '<circle cx="40" cy="40" r="29.5" fill="none" stroke="' . $t['shine'] . '" stroke-width="0.4" opacity="0.25"/>';

    // 6. Dark center (deep)
    $svg .= '<circle cx="40" cy="40" r="29" fill="url(#' . $uid . '-inner)"/>';

    // 7. Inner ring accent: subtle colored border
    $svg .= '<circle cx="40" cy="40" r="29" fill="none" stroke="' . $t['primary'] . '" stroke-width="0.7" opacity="0.45"/>';

    // 8. Icon with glow - CENTERED via translate(40,40)
    $svg .= '<g transform="translate(40,40)" style="color:' . $t['primary'] . '" filter="url(#' . $uid . '-ig)">';
    $svg .= $icon_svg;
    $svg .= '</g>';

    // 9. Specular highlight (top-left shine for 3D depth)
    $svg .= '<ellipse cx="30" cy="26" rx="18" ry="14" fill="url(#' . $uid . '-hl)" opacity="0.5"/>';

    // 10. Bottom reflection (subtle warm bounce)
    $svg .= '<ellipse cx="40" cy="55" rx="14" ry="6" fill="url(#' . $uid . '-ref)"/>';

    // 11. Edge highlight dots (premium coins have these)
    if ($tier === 'gold' || $tier === 'diamond' || $tier === 'legendary') {
        for ($i = 0; $i < 12; $i++) {
            $angle = deg2rad($i * 30);
            $x = 40 + 35.5 * cos($angle);
            $y = 40 + 35.5 * sin($angle);
            $svg .= '<circle cx="' . round($x, 1) . '" cy="' . round($y, 1) . '" r="0.6" fill="' . $t['shine'] . '" opacity="0.4"/>';
        }
    }

    // 12. Legendary crown sparkles
    if ($tier === 'legendary') {
        $svg .= '<circle cx="26" cy="9" r="2" fill="' . $t['primary'] . '" opacity="0.8"><animate attributeName="opacity" values="0.4;1;0.4" dur="2s" repeatCount="indefinite"/></circle>';
        $svg .= '<circle cx="40" cy="5" r="2.8" fill="' . $t['shine'] . '"><animate attributeName="r" values="2.8;3.5;2.8" dur="2s" repeatCount="indefinite"/></circle>';
        $svg .= '<circle cx="54" cy="9" r="2" fill="' . $t['primary'] . '" opacity="0.8"><animate attributeName="opacity" values="0.4;1;0.4" dur="2s" begin="0.5s" repeatCount="indefinite"/></circle>';
    }

    // 13. Diamond sparkle effect
    if ($tier === 'diamond') {
        $svg .= '<polygon points="40,4 41,7 44,7 41.5,9 42.5,12 40,10 37.5,12 38.5,9 36,7 39,7" fill="white" opacity="0.5"><animate attributeName="opacity" values="0.2;0.7;0.2" dur="3s" repeatCount="indefinite"/></polygon>';
    }

    // 14. Tier label banner (admin/large size)
    if ($size >= 70) {
        $svg .= '<rect x="16" y="65" width="48" height="12" rx="6" fill="' . $t['dark'] . '" opacity="0.9"/>';
        $svg .= '<rect x="16" y="65" width="48" height="12" rx="6" fill="none" stroke="' . $t['primary'] . '" stroke-width="0.5" opacity="0.3"/>';
        $svg .= '<text x="40" y="74" text-anchor="middle" fill="' . $t['shine'] . '" font-size="7.5" font-weight="800" font-family="system-ui,-apple-system,sans-serif" letter-spacing="1">' . strtoupper($t['label']) . '</text>';
    }

    $svg .= '</svg>';
    return $svg;
}

/**
 * Get all achievements with their definitions
 * Centralized so both admin and dashboard use the same data
 */
function pianomode_get_all_achievements() {
    return array(
        // --- WELCOME ---
        array('id' => 'newcomer', 'name' => 'Welcome to PianoMode', 'condition' => 'Create an account', 'category' => 'Welcome', 'tier' => 'bronze', 'icon' => 'star'),
        // --- STREAK ---
        array('id' => 'streak_10', 'name' => 'Streak Starter', 'condition' => '10 note streak in Sightreading', 'category' => 'Streak', 'tier' => 'bronze', 'icon' => 'flame'),
        array('id' => 'streak_50', 'name' => 'Streak Legend', 'condition' => '50 note streak in Sightreading', 'category' => 'Streak', 'tier' => 'silver', 'icon' => 'flame'),
        array('id' => 'streak_100', 'name' => 'Streak Immortal', 'condition' => '100 note streak in Sightreading', 'category' => 'Streak', 'tier' => 'gold', 'icon' => 'fire'),
        array('id' => 'streak_week', 'name' => '7-Day Streak', 'condition' => '7 consecutive login days', 'category' => 'Streak', 'tier' => 'silver', 'icon' => 'flame'),
        array('id' => 'streak_month', 'name' => '30-Day Streak', 'condition' => '30 consecutive login days', 'category' => 'Streak', 'tier' => 'gold', 'icon' => 'fire'),
        // --- PRACTICE ---
        array('id' => 'first_session', 'name' => 'First Practice', 'condition' => 'Complete 1 sightreading session', 'category' => 'Practice', 'tier' => 'bronze', 'icon' => 'play'),
        array('id' => 'sessions_10', 'name' => '10 Sessions', 'condition' => 'Complete 10 sightreading sessions', 'category' => 'Practice', 'tier' => 'silver', 'icon' => 'play'),
        array('id' => 'sessions_50', 'name' => 'Practice Veteran', 'condition' => 'Complete 50 sightreading sessions', 'category' => 'Practice', 'tier' => 'gold', 'icon' => 'play'),
        array('id' => 'practice_1h', 'name' => '1 Hour Practice', 'condition' => '1 hour total practice time', 'category' => 'Practice', 'tier' => 'bronze', 'icon' => 'clock'),
        array('id' => 'practice_5h', 'name' => '5 Hours Dedication', 'condition' => '5 hours total practice time', 'category' => 'Practice', 'tier' => 'silver', 'icon' => 'clock'),
        // --- NOTES ---
        array('id' => 'notes_100', 'name' => '100 Notes Played', 'condition' => 'Play 100 notes in Sightreading', 'category' => 'Notes', 'tier' => 'bronze', 'icon' => 'music'),
        array('id' => 'notes_1000', 'name' => '1,000 Notes Master', 'condition' => 'Play 1,000 notes in Sightreading', 'category' => 'Notes', 'tier' => 'silver', 'icon' => 'music'),
        array('id' => 'notes_5000', 'name' => 'Note Virtuoso', 'condition' => 'Play 5,000 notes in Sightreading', 'category' => 'Notes', 'tier' => 'gold', 'icon' => 'music'),
        // --- READING ---
        array('id' => 'reader_5', 'name' => '5 Articles Read', 'condition' => 'Read 5 articles', 'category' => 'Reading', 'tier' => 'bronze', 'icon' => 'book_open'),
        array('id' => 'reader_10', 'name' => '10 Articles Read', 'condition' => 'Read 10 articles', 'category' => 'Reading', 'tier' => 'bronze', 'icon' => 'book_open'),
        array('id' => 'reader_25', 'name' => 'Bookworm', 'condition' => 'Read 25 articles', 'category' => 'Reading', 'tier' => 'silver', 'icon' => 'book_open'),
        array('id' => 'reader_50', 'name' => '50 Articles Master', 'condition' => 'Read 50 articles', 'category' => 'Reading', 'tier' => 'gold', 'icon' => 'book_open'),
        array('id' => 'reader_100', 'name' => 'Library Scholar', 'condition' => 'Read 100 articles', 'category' => 'Reading', 'tier' => 'diamond', 'icon' => 'library'),
        // --- DOWNLOADS ---
        array('id' => 'first_score', 'name' => 'First Score', 'condition' => 'Download 1 score/partition', 'category' => 'Downloads', 'tier' => 'bronze', 'icon' => 'note'),
        array('id' => 'downloader_5', 'name' => '5 Scores Downloaded', 'condition' => 'Download 5 scores', 'category' => 'Downloads', 'tier' => 'bronze', 'icon' => 'note'),
        array('id' => 'downloader_25', 'name' => 'Score Collector', 'condition' => 'Download 25 scores', 'category' => 'Downloads', 'tier' => 'silver', 'icon' => 'note'),
        // --- LEARNING SCORE TIERS ---
        array('id' => 'learning_500', 'name' => 'Silver Treble Clef', 'condition' => 'Reach 500 Learning Score', 'category' => 'Learning Tiers', 'tier' => 'silver', 'icon' => 'treble'),
        array('id' => 'learning_2000', 'name' => 'Gold Treble Clef', 'condition' => 'Reach 2,000 Learning Score', 'category' => 'Learning Tiers', 'tier' => 'gold', 'icon' => 'treble'),
        array('id' => 'learning_5000', 'name' => 'Diamond Treble Clef', 'condition' => 'Reach 5,000 Learning Score', 'category' => 'Learning Tiers', 'tier' => 'diamond', 'icon' => 'treble'),
        // --- GAMING SCORE TIERS ---
        array('id' => 'gaming_500', 'name' => 'Silver Bass Clef', 'condition' => 'Reach 500 Gaming Score', 'category' => 'Gaming Tiers', 'tier' => 'silver', 'icon' => 'bass'),
        array('id' => 'gaming_2000', 'name' => 'Gold Bass Clef', 'condition' => 'Reach 2,000 Gaming Score', 'category' => 'Gaming Tiers', 'tier' => 'gold', 'icon' => 'bass'),
        array('id' => 'gaming_5000', 'name' => 'Diamond Bass Clef', 'condition' => 'Reach 5,000 Gaming Score', 'category' => 'Gaming Tiers', 'tier' => 'diamond', 'icon' => 'bass'),
        // --- COMBINED SCORE ---
        array('id' => 'score_1000', 'name' => 'Rising Star', 'condition' => 'Reach 1,000 combined score', 'category' => 'Milestones', 'tier' => 'bronze', 'icon' => 'piano'),
        array('id' => 'score_5000', 'name' => 'Piano Prodigy', 'condition' => 'Reach 5,000 combined score', 'category' => 'Milestones', 'tier' => 'silver', 'icon' => 'piano'),
        array('id' => 'score_10000', 'name' => 'Maestro', 'condition' => 'Reach 10,000 combined score', 'category' => 'Milestones', 'tier' => 'gold', 'icon' => 'crown'),
        array('id' => 'score_25000', 'name' => 'Grand Maestro', 'condition' => 'Reach 25,000 combined score', 'category' => 'Milestones', 'tier' => 'diamond', 'icon' => 'crown'),
        // --- EAR TRAINER ---
        array('id' => 'et_first_session', 'name' => 'First Listen', 'condition' => 'Complete 1 Ear Trainer session', 'category' => 'Ear Trainer', 'tier' => 'bronze', 'icon' => 'ear'),
        array('id' => 'et_sessions_10', 'name' => '10 Ear Sessions', 'condition' => 'Complete 10 Ear Trainer sessions', 'category' => 'Ear Trainer', 'tier' => 'silver', 'icon' => 'ear'),
        array('id' => 'et_sessions_50', 'name' => 'Ear Training Veteran', 'condition' => 'Complete 50 Ear Trainer sessions', 'category' => 'Ear Trainer', 'tier' => 'gold', 'icon' => 'ear'),
        array('id' => 'et_questions_100', 'name' => '100 Questions Answered', 'condition' => 'Answer 100 Ear Trainer questions', 'category' => 'Ear Trainer', 'tier' => 'bronze', 'icon' => 'ear'),
        array('id' => 'et_questions_500', 'name' => '500 Questions Answered', 'condition' => 'Answer 500 Ear Trainer questions', 'category' => 'Ear Trainer', 'tier' => 'silver', 'icon' => 'ear'),
        array('id' => 'et_streak_10', 'name' => 'Ear Streak', 'condition' => '10 correct streak in Ear Trainer', 'category' => 'Ear Trainer', 'tier' => 'bronze', 'icon' => 'flame'),
        array('id' => 'et_streak_25', 'name' => 'Golden Ear', 'condition' => '25 correct streak in Ear Trainer', 'category' => 'Ear Trainer', 'tier' => 'gold', 'icon' => 'fire'),
        // --- DAILY CHALLENGES ---
        array('id' => 'challenge_7', 'name' => 'Challenge Accepted', 'condition' => 'Complete 7 daily challenges', 'category' => 'Challenges', 'tier' => 'bronze', 'icon' => 'flame'),
        array('id' => 'challenge_30', 'name' => 'Challenge Champion', 'condition' => 'Complete 30 daily challenges', 'category' => 'Challenges', 'tier' => 'silver', 'icon' => 'fire'),
        array('id' => 'challenge_100', 'name' => 'Challenge Master', 'condition' => 'Complete 100 daily challenges', 'category' => 'Challenges', 'tier' => 'gold', 'icon' => 'crown'),
        // --- MULTI-GAME ---
        array('id' => 'explorer', 'name' => 'Game Explorer', 'condition' => 'Play 2 different games', 'category' => 'Multi-Game', 'tier' => 'bronze', 'icon' => 'star'),
        array('id' => 'all_rounder', 'name' => 'All-Rounder', 'condition' => 'Play 4+ different games', 'category' => 'Multi-Game', 'tier' => 'gold', 'icon' => 'trophy'),
        // --- LEVEL UPS ---
        array('id' => 'level_5', 'name' => 'Level 5 Reached', 'condition' => 'Reach level 5', 'category' => 'Level', 'tier' => 'silver', 'icon' => 'treble_sketch'),
        array('id' => 'level_10', 'name' => 'Level 10 Master', 'condition' => 'Reach level 10', 'category' => 'Level', 'tier' => 'gold', 'icon' => 'crown'),
        // --- VISITS ---
        array('id' => 'visitor_10', 'name' => 'Regular Visitor', 'condition' => '10 logins', 'category' => 'Engagement', 'tier' => 'bronze', 'icon' => 'star'),
        array('id' => 'visitor_50', 'name' => 'Loyal Member', 'condition' => '50 logins', 'category' => 'Engagement', 'tier' => 'silver', 'icon' => 'treble'),
        array('id' => 'visitor_100', 'name' => 'PianoMode Devotee', 'condition' => '100 logins', 'category' => 'Engagement', 'tier' => 'gold', 'icon' => 'crown'),
        // --- LEVEL UPS (continued) ---
        array('id' => 'level_20', 'name' => 'Level 20 Legend', 'condition' => 'Reach level 20', 'category' => 'Level', 'tier' => 'legendary', 'icon' => 'level_20'),
        // --- LEGENDARY ---
        array('id' => 'pianomode_master', 'name' => 'PianoMode Master', 'condition' => 'Earn 30+ achievements & reach 50,000 combined score', 'category' => 'Legendary', 'tier' => 'legendary', 'icon' => 'pianomode'),
    );
}

/**
 * Dynamic badge check: runs for the current user on every dashboard/account page load.
 * Awards any badges the user qualifies for but hasn't received yet.
 * Lightweight: only checks the current logged-in user, throttled to once per 60s.
 */
function pianomode_check_user_badges($user_id = null) {
    if (!$user_id) $user_id = get_current_user_id();
    if (!$user_id) return;

    // Throttle: max once per 60 seconds per user
    $last_check = get_user_meta($user_id, 'pm_badge_last_check', true);
    if ($last_check && (time() - (int) $last_check) < 60) return;
    update_user_meta($user_id, 'pm_badge_last_check', time());

    global $wpdb;
    $prefix = $wpdb->prefix;
    $uid = $user_id;

    // newcomer - everyone with an account
    pianomode_retroactive_add($uid, 'newcomer', 'Welcome to PianoMode');

    // Score-based
    $learning = (int) get_user_meta($uid, 'pianomode_learning_score', true);
    $gaming = (int) get_user_meta($uid, 'pianomode_gaming_score', true);
    $total = $learning + $gaming;

    if ($learning >= 500) pianomode_retroactive_add($uid, 'learning_500', 'Silver Treble Clef');
    if ($learning >= 2000) pianomode_retroactive_add($uid, 'learning_2000', 'Gold Treble Clef');
    if ($learning >= 5000) pianomode_retroactive_add($uid, 'learning_5000', 'Diamond Treble Clef');
    if ($gaming >= 500) pianomode_retroactive_add($uid, 'gaming_500', 'Silver Bass Clef');
    if ($gaming >= 2000) pianomode_retroactive_add($uid, 'gaming_2000', 'Gold Bass Clef');
    if ($gaming >= 5000) pianomode_retroactive_add($uid, 'gaming_5000', 'Diamond Bass Clef');
    if ($total >= 1000) pianomode_retroactive_add($uid, 'score_1000', 'Rising Star');
    if ($total >= 5000) pianomode_retroactive_add($uid, 'score_5000', 'Piano Prodigy');
    if ($total >= 10000) pianomode_retroactive_add($uid, 'score_10000', 'Maestro');
    if ($total >= 25000) pianomode_retroactive_add($uid, 'score_25000', 'Grand Maestro');

    // Login count
    $logins = (int) get_user_meta($uid, 'pm_login_count', true);
    if ($logins >= 10) pianomode_retroactive_add($uid, 'visitor_10', 'Regular Visitor');
    if ($logins >= 50) pianomode_retroactive_add($uid, 'visitor_50', 'Loyal Member');
    if ($logins >= 100) pianomode_retroactive_add($uid, 'visitor_100', 'PianoMode Devotee');

    // Note Invaders
    $ni_played = (int) get_user_meta($uid, 'note_invaders_games_played', true);
    // Sightreading - read from user_meta (srt_user_stats) which is the actual data source
    $srt_meta = get_user_meta($uid, 'srt_user_stats', true);
    $sr_sessions = intval($srt_meta['total_sessions'] ?? 0);
    $sr_notes = intval($srt_meta['total_notes_played'] ?? 0);
    $sr_streak = intval($srt_meta['best_streak'] ?? 0);
    $sr_time = intval($srt_meta['total_practice_time'] ?? 0);
    // Fallback: check DB table if user_meta is empty
    if ($sr_sessions === 0) {
        $sr = $wpdb->get_row($wpdb->prepare("SELECT * FROM {$prefix}pm_sightreading_stats WHERE user_id = %d", $uid), ARRAY_A);
        if ($sr) {
            $sr_sessions = max($sr_sessions, intval($sr['total_sessions'] ?? 0));
            $sr_notes = max($sr_notes, intval($sr['total_notes_played'] ?? 0));
            $sr_streak = max($sr_streak, intval($sr['best_streak'] ?? 0));
            $sr_time = max($sr_time, intval($sr['total_practice_time'] ?? 0));
        }
    }

    if ($sr_sessions >= 1) pianomode_retroactive_add($uid, 'first_session', 'First Practice');
    if ($sr_sessions >= 10) pianomode_retroactive_add($uid, 'sessions_10', '10 Sessions');
    if ($sr_sessions >= 50) pianomode_retroactive_add($uid, 'sessions_50', 'Practice Veteran');
    if ($sr_notes >= 100) pianomode_retroactive_add($uid, 'notes_100', '100 Notes Played');
    if ($sr_notes >= 1000) pianomode_retroactive_add($uid, 'notes_1000', '1,000 Notes Master');
    if ($sr_notes >= 5000) pianomode_retroactive_add($uid, 'notes_5000', 'Note Virtuoso');
    if ($sr_streak >= 10) pianomode_retroactive_add($uid, 'streak_10', 'Streak Starter');
    if ($sr_streak >= 50) pianomode_retroactive_add($uid, 'streak_50', 'Streak Legend');
    if ($sr_streak >= 100) pianomode_retroactive_add($uid, 'streak_100', 'Streak Immortal');
    if ($sr_time >= 3600) pianomode_retroactive_add($uid, 'practice_1h', '1 Hour Practice');
    if ($sr_time >= 18000) pianomode_retroactive_add($uid, 'practice_5h', '5 Hours Dedication');

    // Ear Trainer achievements
    $et_stats = get_user_meta($uid, 'pm_ear_trainer_stats', true);
    if (is_array($et_stats)) {
        $et_sessions = intval($et_stats['total_sessions'] ?? 0);
        $et_total_q = intval($et_stats['total_q'] ?? 0);
        $et_streak = intval($et_stats['best_streak'] ?? 0);
        if ($et_sessions >= 1) pianomode_retroactive_add($uid, 'et_first_session', 'First Listen');
        if ($et_sessions >= 10) pianomode_retroactive_add($uid, 'et_sessions_10', '10 Ear Sessions');
        if ($et_sessions >= 50) pianomode_retroactive_add($uid, 'et_sessions_50', 'Ear Training Veteran');
        if ($et_total_q >= 100) pianomode_retroactive_add($uid, 'et_questions_100', '100 Questions Answered');
        if ($et_total_q >= 500) pianomode_retroactive_add($uid, 'et_questions_500', '500 Questions Answered');
        if ($et_streak >= 10) pianomode_retroactive_add($uid, 'et_streak_10', 'Ear Streak');
        if ($et_streak >= 25) pianomode_retroactive_add($uid, 'et_streak_25', 'Golden Ear');
    }

    // User data (reading, downloads, streaks)
    $ud = $wpdb->get_row($wpdb->prepare("SELECT * FROM {$prefix}pm_user_data WHERE user_id = %d", $uid), ARRAY_A);
    if ($ud) {
        $articles = intval($ud['total_articles_read'] ?? 0);
        $downloads = intval($ud['total_scores_downloaded'] ?? 0);
        $streak = max(intval($ud['streak_days'] ?? 0), intval($ud['longest_streak'] ?? 0));

        if ($articles >= 5) pianomode_retroactive_add($uid, 'reader_5', '5 Articles Read');
        if ($articles >= 10) pianomode_retroactive_add($uid, 'reader_10', '10 Articles Read');
        if ($articles >= 25) pianomode_retroactive_add($uid, 'reader_25', 'Bookworm');
        if ($articles >= 50) pianomode_retroactive_add($uid, 'reader_50', '50 Articles Master');
        if ($articles >= 100) pianomode_retroactive_add($uid, 'reader_100', 'Library Scholar');
        if ($downloads >= 1) pianomode_retroactive_add($uid, 'first_score', 'First Score');
        if ($downloads >= 5) pianomode_retroactive_add($uid, 'downloader_5', '5 Scores Downloaded');
        if ($downloads >= 25) pianomode_retroactive_add($uid, 'downloader_25', 'Score Collector');
        if ($streak >= 7) pianomode_retroactive_add($uid, 'streak_week', '7-Day Streak');
        if ($streak >= 30) pianomode_retroactive_add($uid, 'streak_month', '30-Day Streak');
    }

    // Daily challenges
    $challenges_completed = (int) get_user_meta($uid, 'pm_challenges_completed', true);
    if ($challenges_completed >= 7) pianomode_retroactive_add($uid, 'challenge_7', 'Challenge Accepted');
    if ($challenges_completed >= 30) pianomode_retroactive_add($uid, 'challenge_30', 'Challenge Champion');
    if ($challenges_completed >= 100) pianomode_retroactive_add($uid, 'challenge_100', 'Challenge Master');

    // Multi-game
    $ll_played = (int) get_user_meta($uid, 'ledger_line_high_score', true) > 0 ? 1 : 0;
    $et_played = (int) get_user_meta($uid, 'et_best_learning_score', true) > 0 ? 1 : 0;
    $ph_played = (int) get_user_meta($uid, 'ph_best_learning_score', true) > 0 ? 1 : 0;
    $vp_played = (int) get_user_meta($uid, 'vp_total_notes_played', true) > 0 ? 1 : 0;
    $games_tried = ($ni_played > 0 ? 1 : 0) + ($ll_played) + ($sr_sessions > 0 ? 1 : 0) + $et_played + $ph_played + $vp_played;

    if ($games_tried >= 2) pianomode_retroactive_add($uid, 'explorer', 'Game Explorer');
    if ($games_tried >= 4) pianomode_retroactive_add($uid, 'all_rounder', 'All-Rounder');

    // Level
    $level = (int) ($ud['level'] ?? 1);
    if ($level >= 5) pianomode_retroactive_add($uid, 'level_5', 'Level 5 Reached');
    if ($level >= 10) pianomode_retroactive_add($uid, 'level_10', 'Level 10 Master');
    if ($level >= 20) pianomode_retroactive_add($uid, 'level_20', 'Level 20 Legend');

    // Legendary
    $earned_count = (int) $wpdb->get_var($wpdb->prepare("SELECT COUNT(*) FROM {$prefix}pm_achievements WHERE user_id = %d", $uid));
    if ($earned_count >= 30 && $total >= 50000) {
        pianomode_retroactive_add($uid, 'pianomode_master', 'PianoMode Master');
    }
}

/**
 * Hook: check badges dynamically on account/dashboard pages.
 * Also run on wp_login for returning users.
 */
add_action('wp', function() {
    if (is_user_logged_in() && is_page(array('play', 'account', 'mon-compte', 'my-account', 'dashboard', 'tableau-de-bord'))) {
        pianomode_check_user_badges();
    }
});
add_action('wp_login', function($user_login, $user) {
    pianomode_check_user_badges($user->ID);
}, 10, 2);

/**
 * One-time batch: award retroactive badges for ALL existing users.
 * Runs once on admin_init, then sets an option so it never runs again.
 * Safe to leave in code — it's a no-op after first successful run.
 * To re-run: delete option 'pm_retroactive_badges_v3_done' in wp_options.
 */
add_action('admin_init', function() {
    if (get_option('pm_retroactive_badges_v4_done')) return;

    global $wpdb;
    $table = $wpdb->prefix . 'pm_achievements';

    // Ensure table exists using dbDelta
    require_once(ABSPATH . 'wp-admin/includes/upgrade.php');
    $charset_collate = $wpdb->get_charset_collate();
    $sql = "CREATE TABLE {$table} (
        id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
        user_id bigint(20) unsigned NOT NULL,
        achievement_id varchar(50) NOT NULL,
        achievement_name varchar(100) NOT NULL,
        earned_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY  (id),
        UNIQUE KEY user_achievement (user_id, achievement_id),
        KEY user_id (user_id)
    ) {$charset_collate};";
    dbDelta($sql);

    // Reset the static cache in pianomode_retroactive_add so it re-checks
    $user_ids = $wpdb->get_col("SELECT ID FROM {$wpdb->users}");
    foreach ($user_ids as $uid) {
        pianomode_check_user_badges((int) $uid);
    }
    update_option('pm_retroactive_badges_v4_done', time());
});

/**
 * Helper: Add achievement if not already earned
 */
function pianomode_retroactive_add($user_id, $achievement_id, $name) {
    global $wpdb;
    $table = $wpdb->prefix . 'pm_achievements';

    // Ensure table exists (cached per request, but re-check on failure)
    static $table_verified = false;
    if (!$table_verified) {
        $table_exists = $wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $table));
        if (!$table_exists) {
            // Use dbDelta for reliable cross-host table creation
            require_once(ABSPATH . 'wp-admin/includes/upgrade.php');
            $charset_collate = $wpdb->get_charset_collate();
            $sql = "CREATE TABLE {$table} (
                id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
                user_id bigint(20) unsigned NOT NULL,
                achievement_id varchar(50) NOT NULL,
                achievement_name varchar(100) NOT NULL,
                earned_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
                PRIMARY KEY  (id),
                UNIQUE KEY user_achievement (user_id, achievement_id),
                KEY user_id (user_id)
            ) {$charset_collate};";
            dbDelta($sql);
            // Verify creation
            $table_exists = $wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $table));
            if (!$table_exists) {
                // Fallback: try without charset
                $wpdb->query("CREATE TABLE IF NOT EXISTS {$table} (
                    id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
                    user_id bigint(20) unsigned NOT NULL,
                    achievement_id varchar(50) NOT NULL,
                    achievement_name varchar(100) NOT NULL,
                    earned_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    PRIMARY KEY (id),
                    UNIQUE KEY user_achievement (user_id, achievement_id),
                    KEY user_id (user_id)
                )");
            }
        }
        $table_verified = true;
    }

    $exists = $wpdb->get_var($wpdb->prepare(
        "SELECT id FROM {$table} WHERE user_id = %d AND achievement_id = %s",
        $user_id, $achievement_id
    ));
    if (!$exists) {
        $wpdb->insert($table, array(
            'user_id' => $user_id,
            'achievement_id' => $achievement_id,
            'achievement_name' => $name,
            'earned_at' => current_time('mysql')
        ));
    }
}