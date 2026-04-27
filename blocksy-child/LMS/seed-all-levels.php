<?php
/**
 * PianoMode LMS - Seed All Levels (Elementary, Intermediate, Advanced, Expert)
 * Creates 20 modules with all lessons + quiz challenges + resources
 * Run once via: add_action('admin_init', 'pm_seed_all_levels')
 *
 * @package PianoMode
 * @version 1.0
 */

if (!defined('ABSPATH')) exit;

/**
 * Build HTML resources block for a given module slug
 */
function pm_get_module_resources_html($slug) {
    $r = pm_get_all_module_resources();
    if (!isset($r[$slug])) return '';

    $mod = $r[$slug];
    $html = '<div class="pm-lesson-resources" style="background:#1e1e22;border:1px solid #D7BF81;border-radius:12px;padding:24px;margin-top:32px;">';
    $html .= '<h3 style="color:#D7BF81;margin:0 0 16px;font-size:1.05em;">Resources &amp; Practice</h3>';
    $html .= '<div style="display:grid;grid-template-columns:1fr 1fr;gap:16px 24px;">';

    // Articles
    $html .= '<div><p style="margin:0 0 8px;font-weight:700;font-size:0.8em;text-transform:uppercase;letter-spacing:.08em;color:#aaa;">Articles</p>';
    $html .= '<ul style="list-style:none;padding:0;margin:0;display:flex;flex-direction:column;gap:6px;">';
    foreach ($mod['articles'] as $a) {
        $html .= '<li><a href="' . esc_url($a['url']) . '" style="color:#D7BF81;text-decoration:none;font-size:.9em;">' . esc_html($a['title']) . '</a></li>';
    }
    $html .= '</ul></div>';

    // Scores
    $html .= '<div><p style="margin:0 0 8px;font-weight:700;font-size:0.8em;text-transform:uppercase;letter-spacing:.08em;color:#aaa;">Sheet Music</p>';
    $html .= '<ul style="list-style:none;padding:0;margin:0;display:flex;flex-direction:column;gap:6px;">';
    foreach ($mod['scores'] as $s) {
        $html .= '<li><a href="' . esc_url($s['url']) . '" style="color:#D7BF81;text-decoration:none;font-size:.9em;">' . esc_html($s['title']) . '</a></li>';
    }
    $html .= '</ul></div></div>';

    // Games
    $html .= '<div style="margin-top:16px;padding-top:16px;border-top:1px solid rgba(215,191,129,0.2);">';
    $html .= '<p style="margin:0 0 10px;font-weight:700;font-size:0.8em;text-transform:uppercase;letter-spacing:.08em;color:#aaa;">Interactive Practice</p>';
    $html .= '<div style="display:flex;flex-wrap:wrap;gap:8px;">';
    $games_map = [
        'ear_trainer'  => ['url' => '/games/ear-trainer/', 'icon' => "\xF0\x9F\x8E\xB5", 'label' => 'Ear Trainer'],
        'piano_hero'   => ['url' => '/games/piano-hero/',  'icon' => "\xF0\x9F\x8E\xB9", 'label' => 'Piano Hero'],
        'sightreading' => ['url' => '/games/sightreading/','icon' => "\xF0\x9F\x93\x84", 'label' => 'Sightreading'],
        'note_invaders'=> ['url' => '/games/note-invaders/','icon'=>"\xF0\x9F\x9A\x80", 'label' => 'Note Invaders'],
        'virtual_piano'=> ['url' => '/games/virtual-piano/','icon'=>"\xF0\x9F\x8E\xB9", 'label' => 'Virtual Piano'],
    ];
    foreach ($mod['games'] as $g) {
        if (isset($games_map[$g])) {
            $gm = $games_map[$g];
            $html .= '<a href="' . $gm['url'] . '" style="background:rgba(215,191,129,.1);border:1px solid #D7BF81;color:#D7BF81;padding:6px 14px;border-radius:20px;font-size:.85em;text-decoration:none;">' . $gm['icon'] . ' ' . $gm['label'] . '</a>';
        }
    }
    $html .= '</div></div></div>';
    return $html;
}

/**
 * All module resources (articles, scores, games)
 */
function pm_get_all_module_resources() {
    return [
        // ELEMENTARY
        'el-hand-independence' => [
            'articles' => [
                ['title'=>'Developing Hand Independence','url'=>'/explore/piano-learning-tutorials/technique-theory/developing-hand-independence-on-the-piano/'],
                ['title'=>'What is Alberti Bass','url'=>'/explore/piano-learning-tutorials/technique-theory/what-is-alberti-bass/'],
                ['title'=>'Walking Bass Lines','url'=>'/explore/piano-learning-tutorials/technique-theory/walking-bass-lines-piano/'],
            ],
            'scores' => [
                ['title'=>'200 Short Two-Part Canons Op. 14','url'=>'/listen-and-play/200-short-two-part-canons-op-14/'],
                ['title'=>'Bach Minuet in G','url'=>'/listen-and-play/bach-minuet-in-g/'],
                ['title'=>'Clementi Sonatina Op. 36 No. 1','url'=>'/listen-and-play/clementi-sonatina-op-36-no-1/'],
            ],
            'games' => ['piano_hero','ear_trainer','virtual_piano'],
        ],
        'el-scales-arpeggios' => [
            'articles' => [
                ['title'=>'How to Play Piano Scales','url'=>'/explore/piano-learning-tutorials/technique-theory/how-to-play-piano-scales/'],
                ['title'=>'Circle of Fifths Guide','url'=>'/explore/piano-learning-tutorials/technique-theory/circle-of-fifths-piano/'],
                ['title'=>'Piano Scale Fingering','url'=>'/explore/piano-learning-tutorials/technique-theory/piano-scale-fingering/'],
            ],
            'scores' => [
                ['title'=>'Hanon: The Virtuoso Pianist','url'=>'/listen-and-play/hanon-the-virtuoso-pianist/'],
                ['title'=>'12 Melodious Studies Op. 63','url'=>'/listen-and-play/12-melodious-and-very-easy-studies-op-63/'],
            ],
            'games' => ['piano_hero','note_invaders','virtual_piano'],
        ],
        'el-theory-deepening' => [
            'articles' => [
                ['title'=>'Piano Intervals Guide','url'=>'/explore/piano-learning-tutorials/technique-theory/piano-intervals-guide/'],
                ['title'=>'Major and Minor Chords','url'=>'/explore/piano-learning-tutorials/technique-theory/major-and-minor-chords/'],
                ['title'=>'Chord Inversions','url'=>'/explore/piano-learning-tutorials/technique-theory/chord-inversions-piano/'],
                ['title'=>'I-IV-V Progressions','url'=>'/explore/piano-learning-tutorials/technique-theory/i-iv-v-chord-progressions/'],
            ],
            'scores' => [
                ['title'=>'Bach Two-Part Inventions','url'=>'/listen-and-play/bach-two-part-inventions/'],
            ],
            'games' => ['ear_trainer','note_invaders','virtual_piano'],
        ],
        'el-repertoire-building' => [
            'articles' => [
                ['title'=>'How to Learn a New Piano Piece','url'=>'/explore/piano-learning-tutorials/practice-guides/how-to-learn-a-new-piano-piece/'],
                ['title'=>'Baroque Music Basics','url'=>'/explore/piano-learning-tutorials/technique-theory/baroque-piano-music/'],
                ['title'=>'How to Play Dotted Rhythms','url'=>'/explore/piano-learning-tutorials/technique-theory/dotted-rhythms-piano/'],
            ],
            'scores' => [
                ['title'=>'Bach Minuet in G','url'=>'/listen-and-play/bach-minuet-in-g/'],
                ['title'=>'Clementi Sonatina Op. 36 No. 1','url'=>'/listen-and-play/clementi-sonatina-op-36-no-1/'],
                ['title'=>'Greensleeves','url'=>'/listen-and-play/greensleeves/'],
            ],
            'games' => ['piano_hero','sightreading','virtual_piano'],
        ],
        'el-performance-skills' => [
            'articles' => [
                ['title'=>'How to Memorize Piano Music','url'=>'/explore/piano-learning-tutorials/practice-guides/how-to-memorize-piano-music/'],
                ['title'=>'Using the Sustain Pedal','url'=>'/explore/piano-learning-tutorials/technique-theory/how-to-use-sustain-pedal-piano/'],
                ['title'=>'Performance Tips','url'=>'/explore/piano-learning-tutorials/practice-guides/piano-performance-tips/'],
            ],
            'scores' => [
                ['title'=>'Fur Elise','url'=>'/listen-and-play/fur-elise/'],
                ['title'=>'Clementi Sonatina Op. 36 No. 2','url'=>'/listen-and-play/clementi-sonatina-op-36-no-2/'],
            ],
            'games' => ['piano_hero','ear_trainer','virtual_piano'],
        ],
        // INTERMEDIATE
        'in-advanced-scales' => [
            'articles' => [
                ['title'=>'Natural Minor Scale','url'=>'/explore/piano-learning-tutorials/technique-theory/natural-minor-scale-piano/'],
                ['title'=>'Harmonic vs Melodic Minor','url'=>'/explore/piano-learning-tutorials/technique-theory/harmonic-vs-melodic-minor/'],
                ['title'=>'Chromatic Scale Tutorial','url'=>'/explore/piano-learning-tutorials/technique-theory/chromatic-scale-piano/'],
            ],
            'scores' => [
                ['title'=>'Hanon: The Virtuoso Pianist','url'=>'/listen-and-play/hanon-the-virtuoso-pianist/'],
                ['title'=>'Czerny 100 Progressive Studies','url'=>'/listen-and-play/czerny-100-progressive-studies/'],
            ],
            'games' => ['piano_hero','ear_trainer'],
        ],
        'in-harmony' => [
            'articles' => [
                ['title'=>'7th Chords Explained','url'=>'/explore/piano-learning-tutorials/technique-theory/7th-chords-piano/'],
                ['title'=>'ii-V-I Jazz Progression','url'=>'/explore/piano-learning-tutorials/technique-theory/ii-v-i-jazz-progression/'],
                ['title'=>'Voice Leading Basics','url'=>'/explore/piano-learning-tutorials/technique-theory/voice-leading-piano/'],
            ],
            'scores' => [
                ['title'=>'Bach Two-Part Inventions','url'=>'/listen-and-play/bach-two-part-inventions/'],
                ['title'=>'Jazz Standards Vol. 1','url'=>'/listen-and-play/jazz-standards-piano-vol-1/'],
            ],
            'games' => ['ear_trainer','piano_hero','virtual_piano'],
        ],
        'in-reading-mastery' => [
            'articles' => [
                ['title'=>'Syncopation in Piano Music','url'=>'/explore/piano-learning-tutorials/technique-theory/syncopation-piano/'],
                ['title'=>'Compound Time Signatures','url'=>'/explore/piano-learning-tutorials/technique-theory/compound-time-signatures/'],
                ['title'=>'Advanced Sight Reading','url'=>'/explore/piano-learning-tutorials/technique-theory/advanced-sight-reading/'],
            ],
            'scores' => [
                ['title'=>'Bartok Mikrokosmos Vol. 3','url'=>'/listen-and-play/bartok-mikrokosmos-vol-3/'],
                ['title'=>'12 Melodious Studies Op. 63','url'=>'/listen-and-play/12-melodious-and-very-easy-studies-op-63/'],
            ],
            'games' => ['sightreading','note_invaders'],
        ],
        'in-musical-styles' => [
            'articles' => [
                ['title'=>'Classical Period Piano','url'=>'/explore/piano-learning-tutorials/technique-theory/classical-period-piano/'],
                ['title'=>'Introduction to Chopin Style','url'=>'/explore/piano-learning-tutorials/technique-theory/chopin-piano-style/'],
                ['title'=>'How to Play Blues Piano','url'=>'/explore/piano-learning-tutorials/technique-theory/blues-piano/'],
                ['title'=>'Jazz Piano Basics','url'=>'/explore/piano-learning-tutorials/technique-theory/jazz-piano-basics/'],
            ],
            'scores' => [
                ['title'=>'Chopin Prelude Op. 28 No. 7','url'=>'/listen-and-play/chopin-prelude-op-28-no-7/'],
                ['title'=>'Mozart Piano Sonata K. 545','url'=>'/listen-and-play/mozart-piano-sonata-k-545/'],
            ],
            'games' => ['ear_trainer','piano_hero','sightreading'],
        ],
        'in-advanced-technique' => [
            'articles' => [
                ['title'=>'Piano Octave Technique','url'=>'/explore/piano-learning-tutorials/technique-theory/piano-octave-technique/'],
                ['title'=>'How to Play Trills','url'=>'/explore/piano-learning-tutorials/technique-theory/piano-trill-technique/'],
                ['title'=>'Advanced Pedal Technique','url'=>'/explore/piano-learning-tutorials/technique-theory/advanced-pedal-technique/'],
                ['title'=>'Forearm Rotation Piano','url'=>'/explore/piano-learning-tutorials/technique-theory/forearm-rotation-piano/'],
            ],
            'scores' => [
                ['title'=>'Czerny Op. 299: School of Velocity','url'=>'/listen-and-play/czerny-op-299/'],
            ],
            'games' => ['piano_hero','ear_trainer'],
        ],
        // ADVANCED
        'ad-virtuosity' => [
            'articles' => [
                ['title'=>'Four-Octave Scales Piano','url'=>'/explore/piano-learning-tutorials/technique-theory/four-octave-scales-piano/'],
                ['title'=>'Hanon Exercises Guide','url'=>'/explore/piano-learning-tutorials/technique-theory/hanon-exercises-guide/'],
                ['title'=>'Double Notes Piano','url'=>'/explore/piano-learning-tutorials/technique-theory/double-notes-piano/'],
            ],
            'scores' => [
                ['title'=>'Hanon: The Virtuoso Pianist','url'=>'/listen-and-play/hanon-the-virtuoso-pianist/'],
                ['title'=>'Czerny Op. 740','url'=>'/listen-and-play/czerny-op-740/'],
                ['title'=>'Clementi: Gradus ad Parnassum','url'=>'/listen-and-play/clementi-gradus-ad-parnassum/'],
            ],
            'games' => ['piano_hero'],
        ],
        'ad-improvisation' => [
            'articles' => [
                ['title'=>'Modal Improvisation Piano','url'=>'/explore/piano-learning-tutorials/technique-theory/modal-improvisation-piano/'],
                ['title'=>'Dorian Mode Tutorial','url'=>'/explore/piano-learning-tutorials/technique-theory/dorian-mode-piano/'],
                ['title'=>'Jazz Improvisation Basics','url'=>'/explore/piano-learning-tutorials/technique-theory/jazz-piano-improvisation/'],
                ['title'=>'Boogie Woogie Piano','url'=>'/explore/piano-learning-tutorials/technique-theory/boogie-woogie-piano/'],
            ],
            'scores' => [
                ['title'=>'Jazz Standards','url'=>'/listen-and-play/jazz-standards-piano-vol-1/'],
                ['title'=>'Blues Piano Standards','url'=>'/listen-and-play/blues-piano-standards/'],
            ],
            'games' => ['ear_trainer','piano_hero','virtual_piano'],
        ],
        'ad-transcription' => [
            'articles' => [
                ['title'=>'How to Transcribe Music by Ear','url'=>'/explore/piano-learning-tutorials/technique-theory/how-to-transcribe-music/'],
                ['title'=>'Melodic Dictation Techniques','url'=>'/explore/piano-learning-tutorials/technique-theory/melodic-dictation/'],
                ['title'=>'Interval Recognition Advanced','url'=>'/explore/piano-learning-tutorials/technique-theory/interval-recognition-advanced/'],
            ],
            'scores' => [
                ['title'=>'Bach Two-Part Inventions','url'=>'/listen-and-play/bach-two-part-inventions/'],
            ],
            'games' => ['ear_trainer','note_invaders'],
        ],
        'ad-repertoire-mastery' => [
            'articles' => [
                ['title'=>'Chopin Nocturne Op. 9 No. 2','url'=>'/explore/piano-learning-tutorials/song-tutorials/chopin-nocturne-op-9-no-2/'],
                ['title'=>'Beethoven Sonata Guide','url'=>'/explore/piano-learning-tutorials/song-tutorials/beethoven-sonata-guide/'],
                ['title'=>'Debussy Impressionist Technique','url'=>'/explore/piano-learning-tutorials/technique-theory/debussy-piano-technique/'],
                ['title'=>'Joplin Ragtime Piano','url'=>'/explore/piano-learning-tutorials/song-tutorials/joplin-ragtime-piano/'],
            ],
            'scores' => [
                ['title'=>'Chopin Nocturnes Op. 9','url'=>'/listen-and-play/chopin-nocturnes-op-9/'],
                ['title'=>'Moonlight Sonata','url'=>'/listen-and-play/moonlight-sonata/'],
                ['title'=>'Debussy: Suite bergamasque','url'=>'/listen-and-play/debussy-suite-bergamasque/'],
                ['title'=>'Maple Leaf Rag','url'=>'/listen-and-play/maple-leaf-rag/'],
            ],
            'games' => ['piano_hero','ear_trainer','sightreading'],
        ],
        'ad-performance-career' => [
            'articles' => [
                ['title'=>'Concert Performance Preparation','url'=>'/explore/piano-learning-tutorials/practice-guides/concert-performance-preparation/'],
                ['title'=>'Advanced Practice Strategies','url'=>'/explore/piano-learning-tutorials/practice-guides/advanced-practice-strategies/'],
                ['title'=>'Recording Piano at Home','url'=>'/explore/piano-learning-tutorials/practice-guides/recording-piano-at-home/'],
            ],
            'scores' => [
                ['title'=>'Selected Advanced Recital Pieces','url'=>'/listen-and-play/advanced-recital-pieces/'],
            ],
            'games' => ['piano_hero','ear_trainer','sightreading'],
        ],
        // EXPERT
        'ex-masterworks' => [
            'articles' => [
                ['title'=>'Chopin Etudes Op. 10 Guide','url'=>'/explore/piano-learning-tutorials/song-tutorials/chopin-etudes-op-10/'],
                ['title'=>'How to Approach Liszt Etudes','url'=>'/explore/piano-learning-tutorials/song-tutorials/liszt-transcendental-etudes/'],
                ['title'=>'Bach Well-Tempered Clavier Guide','url'=>'/explore/piano-learning-tutorials/technique-theory/bach-well-tempered-clavier/'],
                ['title'=>'Rachmaninoff Piano Technique','url'=>'/explore/piano-learning-tutorials/technique-theory/rachmaninoff-piano-technique/'],
            ],
            'scores' => [
                ['title'=>'Chopin Etudes Op. 10 & 25','url'=>'/listen-and-play/chopin-etudes/'],
                ['title'=>'Liszt Transcendental Etudes','url'=>'/listen-and-play/liszt-transcendental-etudes/'],
                ['title'=>'Bach WTC Book 1','url'=>'/listen-and-play/bach-well-tempered-clavier-book-1/'],
                ['title'=>'Beethoven Late Sonatas','url'=>'/listen-and-play/beethoven-late-sonatas/'],
            ],
            'games' => ['piano_hero','ear_trainer'],
        ],
        'ex-composition' => [
            'articles' => [
                ['title'=>'Species Counterpoint for Pianists','url'=>'/explore/piano-learning-tutorials/technique-theory/species-counterpoint-piano/'],
                ['title'=>'Fugue Writing Basics','url'=>'/explore/piano-learning-tutorials/technique-theory/fugue-writing-piano/'],
                ['title'=>'Piano Arrangement Techniques','url'=>'/explore/piano-learning-tutorials/technique-theory/piano-arrangement-techniques/'],
            ],
            'scores' => [
                ['title'=>'Bach Two-Part Inventions','url'=>'/listen-and-play/bach-two-part-inventions/'],
                ['title'=>'Bach WTC Book 1','url'=>'/listen-and-play/bach-well-tempered-clavier-book-1/'],
            ],
            'games' => ['ear_trainer','virtual_piano'],
        ],
        'ex-teaching-skills' => [
            'articles' => [
                ['title'=>'Advanced Jazz Piano: Upper Structures','url'=>'/explore/piano-learning-tutorials/technique-theory/jazz-upper-structure-triads/'],
                ['title'=>'Piano Pedagogy Methods','url'=>'/explore/piano-learning-tutorials/practice-guides/piano-pedagogy-methods/'],
                ['title'=>'Session Work for Pianists','url'=>'/explore/piano-learning-tutorials/practice-guides/session-work-pianist/'],
            ],
            'scores' => [
                ['title'=>'Advanced Jazz Standards','url'=>'/listen-and-play/advanced-jazz-standards/'],
                ['title'=>'Real Book Jazz Standards','url'=>'/listen-and-play/real-book-jazz-standards/'],
            ],
            'games' => ['ear_trainer','piano_hero'],
        ],
        'ex-professional-dev' => [
            'articles' => [
                ['title'=>'Building a Piano Career','url'=>'/explore/piano-learning-tutorials/practice-guides/piano-career-guide/'],
                ['title'=>'Audition Preparation','url'=>'/explore/piano-learning-tutorials/practice-guides/piano-audition-prep/'],
                ['title'=>'Music Business Basics','url'=>'/explore/piano-learning-tutorials/practice-guides/music-business-basics/'],
                ['title'=>'YouTube Strategy for Musicians','url'=>'/explore/piano-learning-tutorials/practice-guides/youtube-piano-strategy/'],
            ],
            'scores' => [
                ['title'=>'Professional Audition Repertoire','url'=>'/listen-and-play/professional-audition-repertoire/'],
            ],
            'games' => ['piano_hero','ear_trainer','sightreading','virtual_piano'],
        ],
        'ex-specialization' => [
            'articles' => [
                ['title'=>'Jazz Piano Specialization','url'=>'/explore/piano-learning-tutorials/technique-theory/jazz-piano-specialization/'],
                ['title'=>'Classical Piano Competition','url'=>'/explore/piano-learning-tutorials/practice-guides/classical-piano-competition/'],
                ['title'=>'Piano Pedagogy Certification','url'=>'/explore/piano-learning-tutorials/practice-guides/piano-pedagogy-certification/'],
                ['title'=>'Contemporary Piano Techniques','url'=>'/explore/piano-learning-tutorials/technique-theory/contemporary-piano-techniques/'],
            ],
            'scores' => [
                ['title'=>'Bach WTC Book 2','url'=>'/listen-and-play/bach-well-tempered-clavier-book-2/'],
            ],
            'games' => ['ear_trainer','sightreading','piano_hero'],
        ],
    ];
}

/**
 * Main seeding function for all levels beyond Beginner
 */
function pm_seed_all_levels() {
    $seed_version = '1.0';
    if (get_option('pm_seed_all_levels_version') === $seed_version) return;
    if (!current_user_can('manage_options')) return;

    // Load lesson data files from LMS data subfolder
    $data_dir = get_stylesheet_directory() . '/LMS/LMS-data';
    $lesson_files = [
        'pianomode-lessons-em1.php',
        'pianomode-lessons-em2.php',
        'pianomode-lessons-em3-em4-em5.php',
        'pianomode-lessons-im1-im2-im3.php',
        'pianomode-lessons-im4-im5.php',
        'pianomode-lessons-am1-am2-am3.php',
        'pianomode-lessons-am4-am5.php',
        'pianomode-lessons-xm1-xm2-xm3.php',
        'pianomode-lessons-xm4-xm5.php',
    ];
    $loaded = 0;
    foreach ($lesson_files as $lf) {
        $path = $data_dir . '/' . $lf;
        if (file_exists($path)) {
            require_once $path;
            $loaded++;
        }
    }
    if ($loaded === 0) return; // No data files found — bail

    global $wpdb;
    $prefix = $wpdb->prefix;

    // ========================================
    // LEVEL & MODULE DEFINITIONS
    // ========================================
    $levels_config = [
        'elementary' => [
            ['name'=>'Hand Independence','slug'=>'el-hand-independence','desc'=>'Develop true hand independence: contrary and parallel motion, melody/accompaniment balance, walking bass, Alberti bass, polyrhythms, and canon playing.','order'=>1,'fn'=>'pm_get_seed_lessons_elementary_m1'],
            ['name'=>'Scales & Arpeggios','slug'=>'el-scales-arpeggios','desc'=>'Master major scales in multiple keys with proper fingering, thumb tuck technique, and introduction to arpeggios.','order'=>2,'fn'=>'pm_get_seed_lessons_elementary_m2'],
            ['name'=>'Theory Deepening','slug'=>'el-theory-deepening','desc'=>'Intervals, Circle of Fifths, key signatures, triads, inversions, I-IV-V progressions, transposition, and chord symbols.','order'=>3,'fn'=>'pm_get_seed_lessons_elementary_m3'],
            ['name'=>'Repertoire Building','slug'=>'el-repertoire-building','desc'=>'Learn to approach new pieces systematically. Baroque minuets, Classical sonatinas, folk songs, pop classics, and essential notation.','order'=>4,'fn'=>'pm_get_seed_lessons_elementary_m4'],
            ['name'=>'Performance Skills','slug'=>'el-performance-skills','desc'=>'Memorization, self-listening, pedaling, stage presence, expression, duet playing, and Elementary graduation.','order'=>5,'fn'=>'pm_get_seed_lessons_elementary_m5'],
        ],
        'intermediate' => [
            ['name'=>'Advanced Scales','slug'=>'in-advanced-scales','desc'=>'Natural, harmonic, and melodic minor scales in all keys. Chromatic scales, scales in thirds, sixths, and tenths. Speed building.','order'=>1,'fn'=>'pm_get_seed_lessons_intermediate_m1'],
            ['name'=>'Harmony','slug'=>'in-harmony','desc'=>'7th chords, diminished and augmented chords, ii-V-I progressions, voice leading, suspended chords, secondary dominants, and harmonic analysis.','order'=>2,'fn'=>'pm_get_seed_lessons_intermediate_m2'],
            ['name'=>'Reading Mastery','slug'=>'in-reading-mastery','desc'=>'Syncopation, compound time, 6/8 and asymmetric meters, advanced sight reading in both clefs, score navigation, and transposition at sight.','order'=>3,'fn'=>'pm_get_seed_lessons_intermediate_m3'],
            ['name'=>'Musical Styles','slug'=>'in-musical-styles','desc'=>'Classical period, Romantic era, Chopin, blues, jazz, swing rhythm, pop, and ballad playing techniques.','order'=>4,'fn'=>'pm_get_seed_lessons_intermediate_m4'],
            ['name'=>'Advanced Technique','slug'=>'in-advanced-technique','desc'=>'Velocity, double notes, octaves, trills, advanced pedaling, una corda, half-pedaling, forearm rotation, and tone production.','order'=>5,'fn'=>'pm_get_seed_lessons_intermediate_m5'],
        ],
        'advanced' => [
            ['name'=>'Virtuosity','slug'=>'ad-virtuosity','desc'=>'Four-octave scales, Hanon advanced studies, Czerny velocity etudes, double thirds, octave technique, tremolos, polyrhythms, and endurance.','order'=>1,'fn'=>'pm_get_seed_lessons_advanced_m1'],
            ['name'=>'Improvisation','slug'=>'ad-improvisation','desc'=>'Modal improvisation, Dorian and Mixolydian modes, jazz standards, rootless voicings, blues piano, boogie-woogie, and lead sheet performance.','order'=>2,'fn'=>'pm_get_seed_lessons_advanced_m2'],
            ['name'=>'Transcription Skills','slug'=>'ad-transcription','desc'=>'Interval recognition mastery, chord recognition by ear, melodic and rhythmic dictation, full song transcription, and notation software.','order'=>3,'fn'=>'pm_get_seed_lessons_advanced_m3'],
            ['name'=>'Repertoire Mastery','slug'=>'ad-repertoire-mastery','desc'=>'Sonata form analysis, Chopin nocturnes and waltzes, Debussy impressionism, Beethoven sonatas, ragtime, Gershwin, and concert programming.','order'=>4,'fn'=>'pm_get_seed_lessons_advanced_m4'],
            ['name'=>'Performance & Career','slug'=>'ad-performance-career','desc'=>'Concert preparation, advanced practice strategies, recording at home, teaching fundamentals, and building a professional music career.','order'=>5,'fn'=>'pm_get_seed_lessons_advanced_m5'],
        ],
        'expert' => [
            ['name'=>'Masterworks','slug'=>'ex-masterworks','desc'=>'Piano concerto form, Beethoven late sonatas, Chopin etudes, Liszt, Rachmaninoff, Prokofiev, Schubert, Ravel, Scriabin, and Bach WTC.','order'=>1,'fn'=>'pm_get_seed_lessons_expert_m1'],
            ['name'=>'Composition & Arrangement','slug'=>'ex-composition','desc'=>'Species counterpoint, fugue writing, piano arrangement, orchestration basics, film scoring, and original composition.','order'=>2,'fn'=>'pm_get_seed_lessons_expert_m2'],
            ['name'=>'Teaching Skills','slug'=>'ex-teaching-skills','desc'=>'Advanced jazz improvisation, upper structure triads, session work, piano pedagogy methods, and studio teaching.','order'=>3,'fn'=>'pm_get_seed_lessons_expert_m3'],
            ['name'=>'Professional Development','slug'=>'ex-professional-dev','desc'=>'Portfolio career, audition mastery, competition prep, contracts, branding, YouTube strategy, licensing, and long-term planning.','order'=>4,'fn'=>'pm_get_seed_lessons_expert_m4'],
            ['name'=>'Specialization','slug'=>'ex-specialization','desc'=>'Choose your path: Jazz Pro, Classical Pro, Contemporary Pro, or Pedagogy Pro. Historical performance, contemporary techniques, and final recital.','order'=>5,'fn'=>'pm_get_seed_lessons_expert_m5'],
        ],
    ];

    $created_ids = [];

    // ========================================
    // SEED EACH LEVEL
    // ========================================
    foreach ($levels_config as $level_slug => $modules) {
        // Get level term (already created by seed-content.php)
        $level_term = term_exists($level_slug, 'pm_level');
        if (!$level_term) continue;
        $level_id = is_array($level_term) ? $level_term['term_id'] : $level_term;

        foreach ($modules as $mi => $mod) {
            // Create or get module term
            $term = term_exists($mod['slug'], 'pm_module');
            if (!$term) {
                $term = wp_insert_term($mod['name'], 'pm_module', [
                    'slug' => $mod['slug'],
                    'description' => $mod['desc'],
                ]);
            }
            if (is_wp_error($term)) continue;
            $module_id = is_array($term) ? $term['term_id'] : $term;

            // Module meta: ALL modules locked as paid
            update_term_meta($module_id, '_pm_lock_type', 'paid');
            update_term_meta($module_id, '_pm_module_order', $mod['order']);
            update_term_meta($module_id, '_pm_module_level', $level_slug);

            // Get lessons from data function
            $fn = $mod['fn'];
            if (!function_exists($fn)) continue;
            $lessons = $fn();

            // Get resources HTML for this module
            $resources_html = pm_get_module_resources_html($mod['slug']);

            foreach ($lessons as $i => $lesson) {
                // Skip if already exists
                $existing = get_page_by_title($lesson['title'], OBJECT, 'pm_lesson');
                if ($existing) {
                    // Still set lock on L1M1 of each level
                    if ($mi === 0 && $i === 0) {
                        update_post_meta($existing->ID, '_pm_lock_type', 'none');
                    }
                    $created_ids[] = $existing->ID;
                    continue;
                }

                // Enrich content with resources block
                $content = $lesson['content'] . $resources_html;

                $post_id = wp_insert_post([
                    'post_title'   => $lesson['title'],
                    'post_content' => $content,
                    'post_status'  => 'publish',
                    'post_type'    => 'pm_lesson',
                    'post_name'    => sanitize_title($lesson['title']),
                ]);

                if (is_wp_error($post_id)) continue;

                // Assign taxonomies
                wp_set_object_terms($post_id, (int) $level_id, 'pm_level');
                wp_set_object_terms($post_id, (int) $module_id, 'pm_module');

                // Auto-tag
                if (taxonomy_exists('pm_lesson_tag')) {
                    $auto_tags = [];
                    $text = strtolower($lesson['title'] . ' ' . $lesson['content']);
                    $tag_keywords = [
                        'keyboard'=>['keyboard','keys','piano keys'],'notes'=>['notes','note','notation'],
                        'rhythm'=>['rhythm','tempo','beat','time signature'],'chords'=>['chord','triad','harmony'],
                        'scales'=>['scale','major scale','minor scale'],'melody'=>['melody','melodies','tune'],
                        'posture'=>['posture','hand position','finger'],'reading'=>['reading','sight-read','sheet music','staff'],
                        'ear-training'=>['ear train','listening','identify'],'technique'=>['technique','finger exercise','dexterity'],
                        'dynamics'=>['dynamics','forte','piano','volume'],'practice'=>['practice','exercise','drill'],
                        'improvisation'=>['improvise','improvisation','jazz'],'composition'=>['compose','composition','arrangement'],
                        'performance'=>['performance','recital','concert','stage'],
                    ];
                    foreach ($tag_keywords as $tag_slug => $keywords) {
                        foreach ($keywords as $kw) {
                            if (strpos($text, $kw) !== false) { $auto_tags[] = $tag_slug; break; }
                        }
                    }
                    if (!empty($auto_tags)) wp_set_object_terms($post_id, $auto_tags, 'pm_lesson_tag');
                }

                // Lesson meta
                update_post_meta($post_id, '_pm_lesson_order', $i + 1);
                update_post_meta($post_id, '_pm_lesson_duration', $lesson['duration']);
                update_post_meta($post_id, '_pm_lesson_difficulty', $lesson['difficulty']);
                update_post_meta($post_id, '_pm_lesson_xp', $lesson['xp']);
                update_post_meta($post_id, '_pm_lesson_has_quiz', '1');
                if (!empty($lesson['video'])) {
                    update_post_meta($post_id, '_pm_lesson_video', $lesson['video']);
                }
                if (!empty($lesson['interactivity'])) {
                    update_post_meta($post_id, '_pm_lesson_interactivity', $lesson['interactivity']);
                }

                // LOCK: Only L1 of M1 of each level is free
                if ($mi === 0 && $i === 0) {
                    update_post_meta($post_id, '_pm_lock_type', 'none');
                }
                // All others: no explicit lock -> inherit module 'paid'

                // Insert challenges
                if (!empty($lesson['challenges'])) {
                    foreach ($lesson['challenges'] as $ci => $ch) {
                        $wpdb->insert($prefix . 'pm_challenges', [
                            'lesson_id'   => $post_id,
                            'type'        => $ch['type'],
                            'question'    => $ch['question'],
                            'explanation' => $ch['explanation'] ?? '',
                            'sort_order'  => $ci + 1,
                        ]);
                        $challenge_id = $wpdb->insert_id;
                        if (!empty($ch['options'])) {
                            foreach ($ch['options'] as $oi => $opt) {
                                $wpdb->insert($prefix . 'pm_challenge_options', [
                                    'challenge_id' => $challenge_id,
                                    'text'         => $opt['text'],
                                    'is_correct'   => $opt['correct'] ? 1 : 0,
                                    'sort_order'   => $oi + 1,
                                ]);
                            }
                        }
                    }
                }

                $created_ids[] = $post_id;
            }
        }
    }

    // ========================================
    // MODULE REQUIREMENTS (merge with existing)
    // ========================================
    $reqs = get_option('pm_module_requirements', []);
    $new_reqs = [
        'el-hand-independence'  => ['ear_trainer_questions'=>30,'piano_hero_minutes'=>5,'description'=>'Complete 30 Ear Trainer questions + 5 min Piano Hero'],
        'el-scales-arpeggios'   => ['piano_hero_minutes'=>10,'description'=>'Play 10 minutes of Piano Hero scale drills'],
        'el-theory-deepening'   => ['ear_trainer_questions'=>40,'description'=>'Answer 40 Ear Trainer questions on intervals and chords'],
        'el-repertoire-building'=> ['piano_hero_minutes'=>10,'description'=>'Play 10 minutes of Piano Hero repertoire practice'],
        'el-performance-skills' => ['ear_trainer_questions'=>50,'quiz_pass_rate'=>80,'description'=>'Pass all quizzes at 80% + 50 Ear Trainer questions'],
        'in-advanced-scales'    => ['ear_trainer_questions'=>40,'piano_hero_minutes'=>10,'description'=>'40 Ear Trainer questions + 10 min Piano Hero scales'],
        'in-harmony'            => ['ear_trainer_questions'=>50,'description'=>'Answer 50 Ear Trainer questions on chords and progressions'],
        'in-reading-mastery'    => ['sightreading_sessions'=>10,'description'=>'Complete 10 Sightreading game sessions'],
        'in-musical-styles'     => ['ear_trainer_questions'=>40,'piano_hero_minutes'=>10,'description'=>'40 Ear Trainer + 10 min Piano Hero in various styles'],
        'in-advanced-technique' => ['piano_hero_minutes'=>15,'description'=>'Play 15 minutes of advanced Piano Hero drills'],
        'ad-virtuosity'         => ['piano_hero_minutes'=>20,'description'=>'Play 20 minutes of virtuosic Piano Hero exercises'],
        'ad-improvisation'      => ['ear_trainer_questions'=>60,'piano_hero_minutes'=>10,'description'=>'60 Ear Trainer + 10 min Piano Hero improvisation'],
        'ad-transcription'      => ['ear_trainer_questions'=>80,'description'=>'Answer 80 Ear Trainer questions at advanced level'],
        'ad-repertoire-mastery' => ['piano_hero_minutes'=>15,'quiz_pass_rate'=>85,'description'=>'15 min Piano Hero + pass all quizzes at 85%'],
        'ad-performance-career' => ['quiz_pass_rate'=>85,'description'=>'Pass all Advanced quizzes at 85% to graduate'],
        'ex-masterworks'        => ['piano_hero_minutes'=>20,'ear_trainer_questions'=>60,'description'=>'20 min Piano Hero + 60 Ear Trainer at expert level'],
        'ex-composition'        => ['ear_trainer_questions'=>50,'description'=>'Answer 50 Ear Trainer questions on harmony and counterpoint'],
        'ex-teaching-skills'    => ['ear_trainer_questions'=>60,'piano_hero_minutes'=>15,'description'=>'60 Ear Trainer + 15 min Piano Hero jazz session'],
        'ex-professional-dev'   => ['quiz_pass_rate'=>90,'description'=>'Pass all quizzes at 90%'],
        'ex-specialization'     => ['ear_trainer_questions'=>100,'quiz_pass_rate'=>90,'description'=>'100 Ear Trainer + 90% quiz pass rate to graduate Expert'],
    ];
    update_option('pm_module_requirements', array_merge($reqs, $new_reqs));

    // ========================================
    // MODULE FAQS (merge with existing)
    // ========================================
    $faqs = get_option('pm_module_faqs', []);
    $new_faqs = [
        'el-hand-independence' => [
            ['q'=>'What is hand independence?','a'=>'The ability for each hand to play different notes, rhythms, dynamics, and articulations simultaneously.'],
            ['q'=>'Why start with contrary motion?','a'=>'Both hands use the same finger numbers, making it the easiest form of two-hand coordination.'],
            ['q'=>'How long until I develop hand independence?','a'=>'Basic independence develops over 4-8 weeks of daily practice. Full independence is a lifelong pursuit.'],
        ],
        'el-scales-arpeggios' => [
            ['q'=>'How many scales should I learn?','a'=>'Start with C, G, D, F major. Eventually you need all 12 major and 12 minor scales.'],
            ['q'=>'What is the thumb tuck?','a'=>'The thumb passes under fingers 2-3 (or 3-4) to continue the scale smoothly across the keyboard.'],
            ['q'=>'How fast should I play scales?','a'=>'Start at 60 BPM in quarter notes. Build gradually. Professional level is 120+ BPM in sixteenth notes.'],
        ],
        'el-theory-deepening' => [
            ['q'=>'Do I really need music theory?','a'=>'Yes. Theory is to music what grammar is to language. It helps you learn faster, memorize better, and understand what you play.'],
            ['q'=>'What is the Circle of Fifths?','a'=>'A diagram showing the relationship between all 12 keys, organized by ascending fifths. Essential for understanding key signatures.'],
            ['q'=>'What are chord inversions?','a'=>'Playing the same chord notes in a different order (e.g., C-E-G becomes E-G-C or G-C-E).'],
        ],
        'el-repertoire-building' => [
            ['q'=>'How do I choose pieces at my level?','a'=>'Match the difficulty rating. If you cannot play it at 50% tempo after a week, it may be too advanced.'],
            ['q'=>'Should I learn classical or pop music?','a'=>'Both. Classical builds technique and reading. Pop builds chord skills and musical enjoyment.'],
        ],
        'el-performance-skills' => [
            ['q'=>'How do I memorize music?','a'=>'Use multiple memory types: muscle memory, visual memory (picture the score), auditory memory, and analytical memory (understand the harmony).'],
            ['q'=>'When should I start using the pedal?','a'=>'After you can play a piece cleanly without pedal. The pedal should enhance, not cover mistakes.'],
        ],
        'in-advanced-scales' => [
            ['q'=>'Why are there three types of minor scale?','a'=>'Each form serves a different musical purpose: natural for general use, harmonic for strong cadences, melodic for smooth melodic lines.'],
            ['q'=>'Do I need to learn all three forms?','a'=>'Yes. Real music mixes all three forms freely. You need to recognize and play each.'],
        ],
        'in-harmony' => [
            ['q'=>'What makes jazz harmony different?','a'=>'Jazz uses extended chords (7ths, 9ths, 13ths) and complex progressions like ii-V-I instead of simpler I-IV-V.'],
            ['q'=>'What is voice leading?','a'=>'Moving from one chord to the next with minimal hand movement, keeping common tones and moving other notes by step.'],
        ],
        'in-reading-mastery' => [
            ['q'=>'How do I improve sight reading?','a'=>'Practice daily with the Sightreading game. Read music slightly below your playing level. Never stop or go back.'],
            ['q'=>'What is compound time?','a'=>'Time signatures where beats divide into 3 (like 6/8, 9/8, 12/8) instead of 2.'],
        ],
        'in-musical-styles' => [
            ['q'=>'Should I specialize in one style?','a'=>'Not yet. Intermediate level is for exploring all styles. Specialization comes at the Expert level.'],
            ['q'=>'What is swing rhythm?','a'=>'Jazz rhythm where pairs of eighth notes are played long-short instead of evenly. It creates the characteristic jazz feel.'],
        ],
        'in-advanced-technique' => [
            ['q'=>'How do I play octaves without tension?','a'=>'Use wrist bounce and forearm rotation. The wrist acts as a spring. Never grip the octave with finger tension.'],
            ['q'=>'What is half-pedaling?','a'=>'Partially releasing and re-pressing the sustain pedal to partially clear the sound while maintaining some resonance.'],
        ],
        'ad-virtuosity' => [
            ['q'=>'How fast should four-octave scales be?','a'=>'Professional benchmark: all major/minor scales at 120+ BPM in sixteenth notes, 4 octaves, hands together.'],
            ['q'=>'Is Hanon safe to practice?','a'=>'Yes, when played with relaxation, rotation, and musical awareness. Stop immediately if you feel tension or pain.'],
        ],
        'ad-improvisation' => [
            ['q'=>'Can improvisation be learned?','a'=>'Absolutely. It is a skill built through systematic study of scales, chord-scale relationships, and pattern practice.'],
            ['q'=>'What modes should I learn first?','a'=>'Dorian (for minor chords) and Mixolydian (for dominant chords) are the most useful starting points.'],
        ],
        'ad-transcription' => [
            ['q'=>'How do I start transcribing?','a'=>'Begin with simple melodies you know well. Sing the melody, then find it on the piano. Write it down. Check against the original.'],
            ['q'=>'What software should I use?','a'=>'MuseScore (free) is excellent for notation. Audacity (free) can slow down recordings for easier transcription.'],
        ],
        'ad-repertoire-mastery' => [
            ['q'=>'How long to learn a major sonata?','a'=>'A full Classical sonata typically takes 2-6 months of dedicated practice depending on difficulty.'],
            ['q'=>'Should I study music history?','a'=>'Yes. Understanding the historical context transforms your interpretation and deepens your musical understanding.'],
        ],
        'ad-performance-career' => [
            ['q'=>'Can I make a living as a pianist?','a'=>'Yes, through a portfolio career: teaching, performing, accompanying, recording, and session work combined.'],
            ['q'=>'When am I ready to perform publicly?','a'=>'When you can play your program 3 times in a row without major errors and feel comfortable with the music.'],
        ],
        'ex-masterworks' => [
            ['q'=>'Which Chopin etude should I start with?','a'=>'Op. 10 No. 3 (Tristesse) or Op. 25 No. 1 (Aeolian Harp) are the most accessible. Avoid No. 1 and No. 4 until very advanced.'],
            ['q'=>'How do I prepare a concerto?','a'=>'Score study first, then solo part isolation, then orchestral awareness, then integration with a second pianist or recording.'],
        ],
        'ex-composition' => [
            ['q'=>'Do I need to compose to be a good pianist?','a'=>'Not required, but understanding composition deepens your interpretation of all music you play.'],
            ['q'=>'Where do I start with composition?','a'=>'Start with species counterpoint and simple forms (binary, ternary). Then progress to more complex structures.'],
        ],
        'ex-teaching-skills' => [
            ['q'=>'What qualifications do I need to teach?','a'=>'Performance ability, pedagogical knowledge, patience, and communication skills. Formal certification helps but is not always required.'],
        ],
        'ex-professional-dev' => [
            ['q'=>'How do I find performance opportunities?','a'=>'Network at music events, apply to competitions, contact venues directly, create a strong online presence, and collaborate with other musicians.'],
        ],
        'ex-specialization' => [
            ['q'=>'When should I specialize?','a'=>'After completing the broad training of all previous levels. Specialization without a broad foundation limits your career options.'],
            ['q'=>'Can I change my specialization later?','a'=>'Yes. The skills are transferable. Many professional pianists work across multiple specializations.'],
        ],
    ];
    update_option('pm_module_faqs', array_merge($faqs, $new_faqs));

    // ========================================
    // MODULE REFERENCES (merge with existing)
    // ========================================
    $refs = get_option('pm_module_references', []);
    $all_resources = pm_get_all_module_resources();
    foreach ($all_resources as $slug => $res) {
        $refs[$slug] = [
            'articles' => $res['articles'],
            'scores'   => $res['scores'],
        ];
    }
    update_option('pm_module_references', $refs);

    // ========================================
    // DONE
    // ========================================
    update_option('pm_seed_all_levels_version', $seed_version);
    update_option('pm_seed_all_levels_ids', $created_ids);
}

add_action('admin_init', 'pm_seed_all_levels');