<?php
/**
 * PianoMode LMS - Seed Content (Consolidated)
 * Creates all 5 Beginner modules with complete lessons + quiz challenges
 * Modules 1-5: Piano Discovery, Expanding Range, Reading Music, Two Hands Together, First Repertoire
 * Run once via: add_action('admin_init', 'pm_seed_content');
 *
 * @package PianoMode
 * @version 3.0
 */

if (!defined('ABSPATH')) exit;

function pm_get_seed_lessons_module1() {
    return [

        // ====================================================================
        // LESSON 1: Meet the Piano Keyboard
        // ====================================================================
        [
            'title' => 'Meet the Piano Keyboard',
            'duration' => 10,
            'difficulty' => 1,
            'xp' => 50,
            'video' => '',
            'interactivity' => [
                'type' => 'ear_trainer',
                'target' => 5,
                'label' => 'Identify 5 notes by ear'
            ],
            'content' => '
                <h2>Welcome to Your Piano Journey</h2>
                <p>Congratulations on taking the first step toward learning piano. In this lesson, you will discover the layout of the piano keyboard, understand how the keys are organized, and find one of the most important notes in all of music: <strong>Middle C</strong>.</p>
                <p>Whether you have a full 88-key acoustic piano or a smaller digital keyboard, the fundamental layout is the same. Once you understand the pattern, you can navigate any piano in the world.</p>

                <h3>The Black and White Key Pattern</h3>
                <p>Look at your piano keyboard. You will notice two colors of keys: white keys and black keys. The black keys are arranged in a repeating pattern of <strong>groups of two</strong> and <strong>groups of three</strong>.</p>
                <p>This pattern repeats across the entire keyboard. It is the single most important landmark for finding your way around the piano. Every note can be located using these black key groups as a reference.</p>
                <ul>
                    <li>Find a group of <strong>two black keys</strong> anywhere on your keyboard.</li>
                    <li>Now find the next group of <strong>three black keys</strong> to its right.</li>
                    <li>Notice how this pattern, two, three, two, three, repeats all the way across.</li>
                </ul>

                <h3>The Musical Alphabet</h3>
                <p>Music uses the first seven letters of the alphabet: <strong>A, B, C, D, E, F, G</strong>. After G, the pattern starts over at A. These seven letters name the seven white keys in each group.</p>
                <p>Here is how to find each note using the black key groups:</p>
                <ul>
                    <li><strong>C</strong> is the white key immediately to the left of any group of two black keys.</li>
                    <li><strong>D</strong> sits between the two black keys in a group of two.</li>
                    <li><strong>E</strong> is the white key immediately to the right of a group of two black keys.</li>
                    <li><strong>F</strong> is the white key immediately to the left of a group of three black keys.</li>
                    <li><strong>G</strong> sits between the first and second black keys in a group of three.</li>
                    <li><strong>A</strong> sits between the second and third black keys in a group of three.</li>
                    <li><strong>B</strong> is the white key immediately to the right of a group of three black keys.</li>
                </ul>

                <h3>Finding Middle C</h3>
                <p><strong>Middle C</strong> is the C closest to the center of your keyboard. On a full 88-key piano, it is the fourth C from the left. On a 61-key keyboard, it is usually near the middle. In music notation, Middle C is written as <strong>C4</strong>.</p>
                <p>Middle C is your home base. It is the starting point for reading music and the anchor between the treble and bass clefs, which you will learn about in Module 3.</p>

                <h3>What Are Octaves?</h3>
                <p>An <strong>octave</strong> is the distance from one note to the next note with the same name. For example, from one C to the next C above it is one octave. There are 8 white keys in an octave (including both the starting and ending notes), which is why it is called an "octave" (from the Latin word for eight).</p>
                <p>A standard piano has just over seven octaves. A 61-key keyboard has five octaves. Regardless of how many octaves your instrument has, the pattern of notes is identical in each one.</p>

                <h3>Exercise 1: Find Every C</h3>
                <p>Starting from the lowest note on your keyboard, find every C by looking for the white key just to the left of each group of two black keys. Play each one from low to high. Notice how the pitch gets higher as you move to the right.</p>

                <h3>Exercise 2: Name the Notes</h3>
                <p>Pick any group of two black keys on your keyboard. Starting from the C to the left of that group, play and name each white key in order: <strong>C → D → E → F → G → A → B → C</strong>. Repeat this in two or three different places on the keyboard.</p>

                <h3>Why This Matters</h3>
                <p>You might wonder why you need to learn note names before playing songs. Think of it this way: learning note names is like learning the alphabet before reading. You could memorize where a few keys are, but understanding the full layout gives you the freedom to learn any song, read any sheet music, and communicate with other musicians. This foundation will save you hundreds of hours in the long run.</p>
                <p>Professional pianists do not look at the keys when they play. They know the layout so well that their fingers find the right notes automatically. That level of familiarity starts right here, with these simple exercises.</p>

                <h3>Fun Fact</h3>
                <p>The reason piano keys are arranged this way goes back over 700 years. The pattern of black and white keys was designed so that each note has a unique physical feel, you can find any note by touch alone, even with your eyes closed.</p>

                <h3>Practice Tips</h3>
                <p>Spend a few minutes each day simply finding and naming notes on the keyboard. You do not need to play songs yet. The goal right now is to feel comfortable navigating the keys. Try closing your eyes, placing a finger on a random white key, and then guessing which note it is before checking.</p>
                <p>Here is a recommended practice routine for this lesson (5 to 10 minutes total):</p>
                <ol>
                    <li><strong>2 minutes:</strong> Find every C on the keyboard, from lowest to highest.</li>
                    <li><strong>2 minutes:</strong> Starting from any C, name all 7 white keys going up (C, D, E, F, G, A, B).</li>
                    <li><strong>2 minutes:</strong> Pick a random white key. Identify it using the black key groups as landmarks.</li>
                    <li><strong>2 minutes:</strong> With your eyes closed, find C by feeling for the group of 2 black keys. Open your eyes to check.</li>
                </ol>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'How many letters does the musical alphabet use?',
                    'explanation' => 'The musical alphabet uses 7 letters (A through G), then the pattern repeats.',
                    'options' => [
                        ['text' => '5 letters (A through E)', 'correct' => false],
                        ['text' => '7 letters (A through G)', 'correct' => true],
                        ['text' => '8 letters (A through H)', 'correct' => false],
                        ['text' => '12 letters (A through L)', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Where is the note C located on the keyboard?',
                    'explanation' => 'C is always the white key immediately to the left of a group of two black keys.',
                    'options' => [
                        ['text' => 'Between two black keys', 'correct' => false],
                        ['text' => 'To the right of three black keys', 'correct' => false],
                        ['text' => 'To the left of a group of two black keys', 'correct' => true],
                        ['text' => 'To the left of a group of three black keys', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The black keys on the piano are grouped in patterns of:',
                    'explanation' => 'Black keys alternate between groups of two and groups of three across the entire keyboard.',
                    'options' => [
                        ['text' => 'Two and four', 'correct' => false],
                        ['text' => 'Three and four', 'correct' => false],
                        ['text' => 'One and two', 'correct' => false],
                        ['text' => 'Two and three', 'correct' => true],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'If you are sitting at a full 88-key piano and want to find Middle C (C4), which C should you look for?',
                    'explanation' => 'Middle C is the fourth C from the left on a standard 88-key piano, located near the center.',
                    'options' => [
                        ['text' => 'The first C from the left', 'correct' => false],
                        ['text' => 'The fourth C from the left', 'correct' => true],
                        ['text' => 'The last C on the right', 'correct' => false],
                        ['text' => 'Any C on the keyboard', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 2: Posture and Hand Position
        // ====================================================================
        [
            'title' => 'Posture and Hand Position',
            'duration' => 10,
            'difficulty' => 1,
            'xp' => 50,
            'video' => '',
            'interactivity' => null,
            'content' => '
                <h2>Setting Up for Success</h2>
                <p>Before you play a single note, your body position at the piano makes a huge difference. Good posture prevents tension and injury, and it sets the foundation for beautiful tone production. In this lesson, you will learn how to sit, how to position your hands, and how to number your fingers.</p>

                <h3>Bench Height and Distance</h3>
                <p>Sit at the edge of your bench (about the front half), not leaning against a backrest. Your feet should be flat on the floor. Here are the key checkpoints:</p>
                <ul>
                    <li>Your <strong>elbows</strong> should be at the same height as the keyboard, or very slightly above. If your elbows are too low, raise your bench or add a cushion.</li>
                    <li>Your <strong>forearms</strong> should be roughly parallel to the floor.</li>
                    <li>Sit far enough from the keyboard that your elbows are slightly in front of your body, not pressed against your sides.</li>
                    <li>Keep your <strong>shoulders relaxed</strong> and down, not hunched up toward your ears.</li>
                </ul>

                <h3>Finger Numbers</h3>
                <p>In piano playing, each finger has a number. This system is the same for both hands:</p>
                <ul>
                    <li><strong>1</strong> = Thumb</li>
                    <li><strong>2</strong> = Index finger</li>
                    <li><strong>3</strong> = Middle finger</li>
                    <li><strong>4</strong> = Ring finger</li>
                    <li><strong>5</strong> = Pinky (little finger)</li>
                </ul>
                <p>These numbers appear in sheet music to tell you which finger to use on which note. You will see them constantly as you progress.</p>

                <h3>The Hand Shape</h3>
                <p>Imagine you are gently holding a small ball or an orange in your hand. Your fingers should curve naturally, and there should be a rounded arch (or "bridge") across the top of your hand. Here is what to check:</p>
                <ul>
                    <li>Your <strong>fingertips</strong> (not the flat pads) touch the keys.</li>
                    <li>Your <strong>knuckles</strong> form a gentle arch, they should not collapse flat.</li>
                    <li>Your <strong>wrist</strong> should be level with your forearm, not dropping below the keyboard or poking up above it.</li>
                    <li>Your <strong>thumb</strong> rests on its side corner, not flat on the key.</li>
                </ul>

                <h3>Common Mistakes to Avoid</h3>
                <ul>
                    <li><strong>Collapsing knuckles:</strong> When the first knuckle of a finger caves inward instead of staying curved. This weakens your sound and can lead to injury over time.</li>
                    <li><strong>Tense shoulders:</strong> Check in with your shoulders regularly. If they are creeping up, take a breath and let them drop.</li>
                    <li><strong>Flat fingers:</strong> Playing with flat fingers reduces control and produces a weak tone. Keep those fingers curved.</li>
                    <li><strong>Sitting too close or too far:</strong> If your elbows are behind your body, move back. If you are reaching, move forward.</li>
                </ul>

                <h3>Exercise 1: The Posture Check</h3>
                <p>Sit at your piano and go through this checklist:</p>
                <ol>
                    <li>Feet flat on the floor.</li>
                    <li>Sitting on the front half of the bench.</li>
                    <li>Elbows at keyboard height or slightly above.</li>
                    <li>Shoulders relaxed and down.</li>
                    <li>Forearms roughly parallel to the floor.</li>
                </ol>
                <p>Hold this position for 30 seconds, then relax and do it again. Make this check a habit every time you sit down to practice.</p>

                <h3>Exercise 2: The Finger Tap</h3>
                <p>Place your right hand on a flat surface (a table is fine). Curve your fingers as if holding an orange. Now tap each finger one at a time, starting with your thumb (1) and going to your pinky (5), then reverse: 5, 4, 3, 2, 1. Keep the non-tapping fingers still. Repeat with your left hand.</p>
                <p>This exercise builds finger independence without needing to play notes yet. Do 5 rounds with each hand.</p>

                <h3>Why Posture Matters So Much</h3>
                <p>You might feel tempted to skip the posture lesson and jump straight to playing notes. But consider this: professional pianists practice for hours every day. The only way they can do that without pain or injury is through excellent posture habits they built from the very beginning.</p>
                <p>Bad posture creates tension. Tension creates pain. Pain forces you to stop practicing. And stopping practice means slower progress. Good posture is not just about comfort, it directly affects the quality of your sound and how fast you improve.</p>

                <h3>Exercise 3: The 30-Second Body Scan</h3>
                <p>This is a technique professional musicians use before every practice session:</p>
                <ol>
                    <li>Sit at your piano in proper position.</li>
                    <li>Close your eyes and take two slow, deep breaths.</li>
                    <li>Mentally scan from your head down: Is your jaw relaxed? Shoulders down? Elbows free? Wrists level? Fingers curved?</li>
                    <li>If you find tension anywhere, consciously relax that area.</li>
                    <li>Open your eyes. You are ready to play.</li>
                </ol>
                <p>Make this a habit before every practice session. It takes 30 seconds and dramatically improves your playing experience.</p>

                <h3>Recommended Practice Routine</h3>
                <p>For this lesson, your practice routine should be 5 to 8 minutes:</p>
                <ol>
                    <li><strong>1 minute:</strong> Posture check (the 5-point checklist above).</li>
                    <li><strong>2 minutes:</strong> 30-second body scan, then hold position while breathing naturally.</li>
                    <li><strong>3 minutes:</strong> Finger taps on a flat surface (5 rounds per hand).</li>
                    <li><strong>2 minutes:</strong> Place hands on the keyboard in curved position. Press a few keys gently, focusing only on hand shape.</li>
                </ol>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'Which finger is number 1 in piano finger numbering?',
                    'explanation' => 'Finger 1 is the thumb on both hands. This is universal in piano fingering.',
                    'options' => [
                        ['text' => 'Pinky', 'correct' => false],
                        ['text' => 'Index finger', 'correct' => false],
                        ['text' => 'Thumb', 'correct' => true],
                        ['text' => 'Middle finger', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Where should your elbows be when sitting at the piano?',
                    'explanation' => 'Your elbows should be at keyboard height or slightly above for proper alignment and relaxed playing.',
                    'options' => [
                        ['text' => 'Well below the keyboard', 'correct' => false],
                        ['text' => 'At keyboard height or slightly above', 'correct' => true],
                        ['text' => 'As high as your shoulders', 'correct' => false],
                        ['text' => 'Pressed tightly against your sides', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What is a common beginner mistake related to hand shape?',
                    'explanation' => 'Collapsing knuckles (first knuckle caving inward) weakens tone and can lead to injury.',
                    'options' => [
                        ['text' => 'Keeping fingers too curved', 'correct' => false],
                        ['text' => 'Holding the wrist too high', 'correct' => false],
                        ['text' => 'Collapsing the knuckles inward', 'correct' => true],
                        ['text' => 'Using fingertips instead of finger pads', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'A student notices tension in their shoulders after 5 minutes of playing. What should they do first?',
                    'explanation' => 'Taking a breath and consciously dropping the shoulders is the immediate fix for shoulder tension.',
                    'options' => [
                        ['text' => 'Stop playing and take a break for the day', 'correct' => false],
                        ['text' => 'Take a breath, consciously drop the shoulders, and continue', 'correct' => true],
                        ['text' => 'Raise the bench higher', 'correct' => false],
                        ['text' => 'Play louder to push through the tension', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 3: Playing Your First Notes
        // ====================================================================
        [
            'title' => 'Playing Your First Notes',
            'duration' => 12,
            'difficulty' => 2,
            'xp' => 60,
            'video' => '',
            'interactivity' => [
                'type' => 'piano_hero',
                'target' => 3,
                'label' => 'Practice C-D-E in Piano Hero for 3 minutes'
            ],
            'content' => '
                <h2>Your First Notes: C, D, and E</h2>
                <p>It is time to make some music. In this lesson, you will place your right hand in the <strong>C position</strong> and play your first three notes: <strong>C, D, and E</strong>. By the end of this lesson, you will be able to play a simple pattern with these notes.</p>

                <h3>The C Position (Right Hand)</h3>
                <p>Find <strong>Middle C</strong> on your keyboard (the C closest to the center, to the left of a group of two black keys). Now place your right hand fingers as follows:</p>
                <ul>
                    <li><strong>Finger 1</strong> (thumb) on <strong>C</strong></li>
                    <li><strong>Finger 2</strong> (index) on <strong>D</strong></li>
                    <li><strong>Finger 3</strong> (middle) on <strong>E</strong></li>
                    <li><strong>Finger 4</strong> (ring) on <strong>F</strong> (you will use this next lesson)</li>
                    <li><strong>Finger 5</strong> (pinky) on <strong>G</strong> (you will use this next lesson)</li>
                </ul>
                <p>Each finger sits on one white key, with no gaps. This is called the <strong>C position</strong> or <strong>C five-finger position</strong>. Your hand should maintain the curved, relaxed shape you learned in the previous lesson.</p>

                <h3>Playing C, D, and E</h3>
                <p>Press each key one at a time, using a firm but gentle touch. Let each note ring out clearly before playing the next one. Here is what to focus on:</p>
                <ul>
                    <li>Press the key with your <strong>fingertip</strong>, not the flat part of the finger.</li>
                    <li>Use a <strong>downward motion</strong> from the finger, not a slapping motion from the wrist.</li>
                    <li>Keep the other fingers resting lightly on their keys, do not lift them up in the air.</li>
                    <li>Listen for a clear, even sound on each note.</li>
                </ul>
                <p>Play this pattern slowly: <strong>C → D → E → D → C</strong>. Repeat it five times.</p>

                <h3>Touch Technique: Pressing vs. Striking</h3>
                <p>A common mistake for beginners is to "strike" or "hit" the keys from above. Instead, think of <strong>pressing into</strong> the key, as if you are gently pushing it down to the bottom. The key should go all the way down smoothly.</p>
                <p>The volume you produce depends on how fast and firmly you press the key, not on how high you lift your finger before pressing. A controlled, gentle press will produce a beautiful tone.</p>

                <h3>Exercise 1: The C-D-E Climb</h3>
                <p>Play the following pattern with your right hand, one note at a time, at a slow and steady pace:</p>
                <ol>
                    <li><strong>C → D → E</strong> (going up)</li>
                    <li><strong>E → D → C</strong> (going back down)</li>
                </ol>
                <p>Repeat this 10 times. Focus on even tone and keeping your hand relaxed. Count "1, 2, 3" as you play each group.</p>

                <h3>Exercise 2: Simple Pattern</h3>
                <p>Try this slightly longer pattern:</p>
                <p><strong>C → C → D → D → E → E → D → D → C</strong></p>
                <p>Each note gets one count. Try counting out loud as you play: "1, 2, 3, 4, 5, 6, 7, 8, 9." Repeat until it feels comfortable and even.</p>

                <h3>What Good Tone Sounds Like</h3>
                <p>As you play C, D, and E, listen carefully to the sound each note produces. A good tone is:</p>
                <ul>
                    <li><strong>Clear:</strong> Each note rings out distinctly, without buzzing or muffled sound.</li>
                    <li><strong>Even:</strong> All three notes should be roughly the same volume.</li>
                    <li><strong>Sustained:</strong> The note should sing until you deliberately lift your finger.</li>
                </ul>
                <p>If a note sounds weak or choked, you are probably not pressing the key all the way down. If it sounds harsh, you may be striking instead of pressing. Aim for a warm, singing quality.</p>

                <h3>Practice Tips</h3>
                <p>Do not rush. Speed is never the goal at this stage, accuracy and relaxation are. If you notice tension creeping into your hand or shoulder, stop, shake out your hands gently, reset your posture, and start again. Even professional pianists check in with their body regularly.</p>

                <h3>Recommended Practice Routine (10 minutes)</h3>
                <ol>
                    <li><strong>1 minute:</strong> Posture check and 30-second body scan from the previous lesson.</li>
                    <li><strong>3 minutes:</strong> Play C, D, E one at a time, slowly. Focus on curved fingers and even tone.</li>
                    <li><strong>3 minutes:</strong> Play the C-D-E Climb pattern (Exercise 1) 10 times.</li>
                    <li><strong>3 minutes:</strong> Play the Simple Pattern (Exercise 2). Count out loud as you play.</li>
                </ol>
                <p>If your hand feels tired or tense at any point, take a 30-second break. Shake your hands gently and reset. Short, focused practice is far more effective than long, tense practice.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'In the right hand C position, which finger plays the note D?',
                    'explanation' => 'In C position, finger 2 (index finger) plays D, since finger 1 (thumb) is on C.',
                    'options' => [
                        ['text' => 'Finger 1 (thumb)', 'correct' => false],
                        ['text' => 'Finger 2 (index)', 'correct' => true],
                        ['text' => 'Finger 3 (middle)', 'correct' => false],
                        ['text' => 'Finger 4 (ring)', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What part of your finger should touch the key when playing?',
                    'explanation' => 'The fingertip (curved, not flat) produces the best control and tone quality.',
                    'options' => [
                        ['text' => 'The flat pad of the finger', 'correct' => false],
                        ['text' => 'The fingernail', 'correct' => false],
                        ['text' => 'The fingertip (curved)', 'correct' => true],
                        ['text' => 'The knuckle', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'How should you produce sound on the piano?',
                    'explanation' => 'Pressing the key smoothly downward gives better control than striking from above.',
                    'options' => [
                        ['text' => 'Strike the key hard from high above', 'correct' => false],
                        ['text' => 'Slap the key with your wrist', 'correct' => false],
                        ['text' => 'Lift your finger as high as possible before hitting', 'correct' => false],
                        ['text' => 'Press gently into the key with a controlled downward motion', 'correct' => true],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'A student is playing C-D-E but notices that E sounds much louder than C and D. What is the most likely cause?',
                    'explanation' => 'Uneven volume usually means uneven finger pressure. The middle finger (3) is naturally stronger, requiring conscious control.',
                    'options' => [
                        ['text' => 'The piano is broken', 'correct' => false],
                        ['text' => 'The middle finger is naturally stronger, pressing harder than the others', 'correct' => true],
                        ['text' => 'E is always a louder note than C', 'correct' => false],
                        ['text' => 'They are sitting too close to the piano', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 4: Completing the C Position
        // ====================================================================
        [
            'title' => 'Completing the C Position',
            'duration' => 12,
            'difficulty' => 2,
            'xp' => 60,
            'video' => '',
            'interactivity' => [
                'type' => 'piano_hero',
                'target' => 3,
                'label' => 'Practice Mary Had a Little Lamb for 3 minutes'
            ],
            'content' => '
                <h2>Adding F and G: The Complete C Position</h2>
                <p>You already know C, D, and E. Now it is time to add the final two notes in the C position: <strong>F</strong> and <strong>G</strong>. With all five fingers placed on five consecutive white keys, you have everything you need to play your first real melody.</p>

                <h3>Placing Fingers 4 and 5</h3>
                <p>Your right hand should already be in C position with fingers 1, 2, and 3 on C, D, and E. Now make sure:</p>
                <ul>
                    <li><strong>Finger 4</strong> (ring finger) rests on <strong>F</strong></li>
                    <li><strong>Finger 5</strong> (pinky) rests on <strong>G</strong></li>
                </ul>
                <p>The ring finger and pinky are naturally weaker than the other fingers. This is completely normal. They will gain strength and independence with practice. Pay extra attention to keeping a curved shape and pressing firmly enough to produce a clear sound.</p>

                <h3>The Five-Note Scale</h3>
                <p>Play all five notes going up and then back down:</p>
                <p><strong>C → D → E → F → G → F → E → D → C</strong></p>
                <p>This is called a <strong>five-finger pattern</strong> or <strong>pentascale</strong>. It is one of the most fundamental patterns in piano playing. Keep each note even in volume and duration. Count steadily as you play.</p>

                <h3>Your First Song: Mary Had a Little Lamb</h3>
                <p>You now know enough notes to play a real melody. Here is "Mary Had a Little Lamb" using the notes of the C position. The numbers represent your right hand fingers:</p>
                <p><strong>E  D  C  D  |  E  E  E  (hold)  |  D  D  D  (hold)  |  E  G  G  (hold)</strong></p>
                <p><strong>E  D  C  D  |  E  E  E  E  |  D  D  E  D  |  C  (hold)</strong></p>
                <p>Each note gets one count, and "(hold)" means hold for one extra count. Play this very slowly at first, making sure each note is clear. Once you can play it without mistakes, gradually increase your speed.</p>

                <h3>Finger Independence Check</h3>
                <p>A common challenge at this stage is that when finger 4 plays, finger 3 or 5 may try to move along with it. This is because the tendons in your hand connect these fingers. With practice, you will gain more independence. Here is a quick exercise:</p>
                <ul>
                    <li>Place all five fingers in C position.</li>
                    <li>Play only finger 4 (F) five times, keeping all other fingers still on their keys.</li>
                    <li>Now play only finger 5 (G) five times, keeping all other fingers still.</li>
                    <li>Alternate: finger 4, finger 5, finger 4, finger 5, ten times.</li>
                </ul>

                <h3>Exercise 1: Five-Finger Warm-Up</h3>
                <p>Play this pattern three times:</p>
                <p><strong>C → D → E → F → G → F → E → D → C</strong> (ascending and descending)</p>
                <p>Then play it again, but this time try to make every note exactly the same volume. Listen carefully.</p>

                <h3>Exercise 2: Play Mary Had a Little Lamb</h3>
                <p>Practice the melody written above at least five times. On the first two attempts, go very slowly and say the note names out loud as you play. On the next three attempts, try to play a bit more smoothly without stopping between notes.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'In the C position (right hand), which finger plays F?',
                    'explanation' => 'Finger 4 (ring finger) plays F in the C position, where each finger covers one consecutive white key starting from C.',
                    'options' => [
                        ['text' => 'Finger 3 (middle)', 'correct' => false],
                        ['text' => 'Finger 5 (pinky)', 'correct' => false],
                        ['text' => 'Finger 4 (ring)', 'correct' => true],
                        ['text' => 'Finger 2 (index)', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What is a five-finger pattern (pentascale)?',
                    'explanation' => 'A pentascale (five-finger pattern) is a sequence of five consecutive notes, one per finger, going up and back down.',
                    'options' => [
                        ['text' => 'A scale using all the black keys', 'correct' => false],
                        ['text' => 'Five consecutive notes, one per finger', 'correct' => true],
                        ['text' => 'A chord with five notes', 'correct' => false],
                        ['text' => 'A pattern that uses only the thumb', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Which notes from the C position are used in "Mary Had a Little Lamb"?',
                    'explanation' => 'Mary Had a Little Lamb uses C, D, E, and G. All notes of the C position except F.',
                    'options' => [
                        ['text' => 'C, D, and E only', 'correct' => false],
                        ['text' => 'All five notes: C, D, E, F, G', 'correct' => false],
                        ['text' => 'C, D, E, and G', 'correct' => true],
                        ['text' => 'D, E, F, and G only', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'When you play finger 4 (F), your finger 3 (E) also moves. What is the best approach to fix this?',
                    'explanation' => 'Practicing isolated finger movements slowly builds independence. The tendons connecting fingers 3, 4, and 5 require targeted training.',
                    'options' => [
                        ['text' => 'Press harder on finger 4', 'correct' => false],
                        ['text' => 'Practice isolated finger 4 movements slowly, keeping other fingers still', 'correct' => true],
                        ['text' => 'Skip the notes that use finger 4', 'correct' => false],
                        ['text' => 'Use finger 3 instead of finger 4 for F', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 5: Understanding Basic Rhythm
        // ====================================================================
        [
            'title' => 'Understanding Basic Rhythm',
            'duration' => 15,
            'difficulty' => 3,
            'xp' => 70,
            'video' => '',
            'interactivity' => [
                'type' => 'ear_trainer',
                'target' => 5,
                'label' => 'Identify 5 note durations by ear'
            ],
            'content' => '
                <h2>The Heartbeat of Music: Rhythm</h2>
                <p>Music is not just about which notes you play, it is equally about <strong>when</strong> and <strong>how long</strong> you play them. Rhythm is the pattern of long and short sounds that gives music its sense of movement and energy. In this lesson, you will learn about note durations and how to count beats.</p>

                <h3>The Steady Beat (Pulse)</h3>
                <p>Every piece of music has a <strong>pulse</strong>, a steady, underlying beat, like a heartbeat. When you tap your foot to a song, you are feeling the pulse. The pulse does not speed up or slow down (unless the music intentionally changes tempo).</p>
                <p>Try this: clap your hands at a steady pace, about once per second. That is a pulse. Each clap represents one <strong>beat</strong>.</p>

                <h3>Note Values: How Long Each Note Lasts</h3>
                <p>Different types of notes last for different numbers of beats. Here are the three most important note values for beginners:</p>
                <ul>
                    <li><strong>Whole note</strong> = 4 beats. You play the note and hold it for a count of four. Count: "1 - 2 - 3 - 4."</li>
                    <li><strong>Half note</strong> = 2 beats. You play the note and hold it for a count of two. Count: "1 - 2."</li>
                    <li><strong>Quarter note</strong> = 1 beat. You play the note and hold it for a count of one. Count: "1."</li>
                </ul>
                <p>Think of it like this: one whole note equals two half notes, and one half note equals two quarter notes. So one whole note equals four quarter notes. The math is built into the names.</p>

                <h3>Counting Out Loud</h3>
                <p>One of the most effective practice techniques at any level is <strong>counting out loud</strong> while you play. It might feel awkward at first, but it trains your brain to keep a steady tempo and understand how notes fit together rhythmically.</p>
                <p>For a pattern in 4/4 time (which means 4 beats per measure), you would count: <strong>"1, 2, 3, 4, 1, 2, 3, 4"</strong> and so on, repeating every four beats.</p>

                <h3>Rests: The Sound of Silence</h3>
                <p>A <strong>rest</strong> is a moment of silence in music. Just like notes, rests have specific durations:</p>
                <ul>
                    <li><strong>Whole rest</strong> = 4 beats of silence</li>
                    <li><strong>Half rest</strong> = 2 beats of silence</li>
                    <li><strong>Quarter rest</strong> = 1 beat of silence</li>
                </ul>
                <p>Rests are just as important as notes. Do not rush through them. Count the rest silently and wait for the correct number of beats before playing the next note.</p>

                <h3>Exercise 1: Rhythm Clapping</h3>
                <p>Without playing the piano, clap these rhythm patterns while counting out loud:</p>
                <ol>
                    <li>Four quarter notes: <strong>clap - clap - clap - clap</strong> (count "1, 2, 3, 4")</li>
                    <li>Two half notes: <strong>clap (hold) clap (hold)</strong> (count "1, 2, 3, 4", clap on 1 and 3)</li>
                    <li>One whole note: <strong>clap (hold, hold, hold)</strong> (count "1, 2, 3, 4", clap on 1 only)</li>
                    <li>Mix: <strong>quarter, quarter, half note</strong> (clap on 1, 2, then hold 3-4)</li>
                </ol>

                <h3>Exercise 2: Playing with Rhythm</h3>
                <p>Now go to your piano. Place your right hand in C position. Play the following, counting out loud:</p>
                <ol>
                    <li>Play C as a whole note: press C and count "1, 2, 3, 4" before releasing.</li>
                    <li>Play D as two half notes: press D on "1" (hold for "1, 2"), press D again on "3" (hold for "3, 4").</li>
                    <li>Play E as four quarter notes: press E on each beat, "1, 2, 3, 4."</li>
                </ol>
                <p>Now try the beginning of "Ode to Joy" using quarter notes (each note gets one beat):</p>
                <p><strong>E  E  F  G  |  G  F  E  D  |  C  C  D  E  |  E (hold) D (hold)</strong></p>
                <p>The "(hold)" means to hold for an extra beat (those are half notes). Count steadily and keep the tempo even.</p>

                <h3>Understanding Measures (Bars)</h3>
                <p>Music is divided into equal groups of beats called <strong>measures</strong> (also called <strong>bars</strong>). In the examples above, the vertical lines ( | ) separate measures. Most beginner music uses <strong>4/4 time</strong>, which means 4 beats per measure.</p>
                <p>Think of measures like sentences in a book. Just as sentences organize words into meaningful groups, measures organize beats into rhythmic groups. Each measure has exactly the same number of beats.</p>

                <h3>A Real-World Analogy</h3>
                <p>Rhythm in music works like rhythm in speech. When you say "MA-ry HAD a LIT-tle LAMB," some syllables are naturally longer or more emphasized than others. Music works the same way: some notes are long, some are short, and the pattern creates the groove and feeling of the piece.</p>

                <h3>Exercise 3: The Rhythm Challenge</h3>
                <p>Try this combination on your piano in C position:</p>
                <ol>
                    <li>Play C as a half note (hold for 2 beats), then D as a half note (hold for 2 beats). That fills one measure.</li>
                    <li>Play E, F, G, E as four quarter notes (1 beat each). That fills one measure.</li>
                    <li>Play C as a whole note (hold for 4 beats). That fills one measure by itself.</li>
                </ol>
                <p>Repeat this three-measure pattern 5 times. Count out loud the entire time: "1, 2, 3, 4" for each measure.</p>

                <h3>Practice Tips</h3>
                <p>Use a metronome (there is one built into PianoMode) set to 60 BPM (beats per minute) for your first rhythm exercises. One beat per second is a comfortable starting tempo. As you get more comfortable, you can gradually increase the speed to 72 or 80 BPM.</p>

                <h3>Recommended Practice Routine (12 minutes)</h3>
                <ol>
                    <li><strong>2 minutes:</strong> Rhythm clapping (Exercise 1) without the piano.</li>
                    <li><strong>3 minutes:</strong> Whole notes, half notes, and quarter notes on C (Exercise 2, items 1-3).</li>
                    <li><strong>4 minutes:</strong> "Ode to Joy" opening with counting out loud.</li>
                    <li><strong>3 minutes:</strong> The Rhythm Challenge (Exercise 3).</li>
                </ol>
                <p>Remember: rhythm is a skill that improves dramatically with consistent practice. Even 5 minutes of daily rhythm work will make a noticeable difference within a week.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'How many beats does a half note receive?',
                    'explanation' => 'A half note receives 2 beats. It is half the length of a whole note (4 beats).',
                    'options' => [
                        ['text' => '1 beat', 'correct' => false],
                        ['text' => '2 beats', 'correct' => true],
                        ['text' => '3 beats', 'correct' => false],
                        ['text' => '4 beats', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'How many quarter notes fit in the same time as one whole note?',
                    'explanation' => 'A whole note lasts 4 beats, and each quarter note lasts 1 beat, so 4 quarter notes equal one whole note.',
                    'options' => [
                        ['text' => '2 quarter notes', 'correct' => false],
                        ['text' => '3 quarter notes', 'correct' => false],
                        ['text' => '4 quarter notes', 'correct' => true],
                        ['text' => '8 quarter notes', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What is a rest in music?',
                    'explanation' => 'A rest is a period of measured silence. Like notes, rests have specific durations and must be counted.',
                    'options' => [
                        ['text' => 'A note played very softly', 'correct' => false],
                        ['text' => 'A break where the pianist stops to relax', 'correct' => false],
                        ['text' => 'The end of a piece of music', 'correct' => false],
                        ['text' => 'A measured period of silence with a specific duration', 'correct' => true],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You are playing a piece with quarter notes but keep speeding up without realizing it. What is the best tool to help you stay steady?',
                    'explanation' => 'A metronome provides an audible click at a set tempo, helping you maintain a consistent pulse.',
                    'options' => [
                        ['text' => 'Play louder to hear yourself better', 'correct' => false],
                        ['text' => 'Use a metronome set to your target tempo', 'correct' => true],
                        ['text' => 'Play with your eyes closed', 'correct' => false],
                        ['text' => 'Only practice whole notes', 'correct' => false],
                    ]
                ],
            ]
        ],

    ]; // end Module 1 lessons
}


// ============================================================================
// MODULE 2: EXPANDING RANGE
// ============================================================================

function pm_get_seed_lessons_module2() {
    return [

        // ====================================================================
        // LESSON 1: The G Position
        // ====================================================================
        [
            'title' => 'The G Position',
            'duration' => 12,
            'difficulty' => 2,
            'xp' => 60,
            'video' => '',
            'interactivity' => [
                'type' => 'ear_trainer',
                'target' => 5,
                'label' => 'Identify 5 notes (C through B) by ear'
            ],
            'content' => '
                <h2>Moving to the G Position</h2>
                <p>In the previous module, you learned the C position, covering the notes <strong>C, D, E, F, and G</strong>. Now it is time to expand your range by learning the <strong>G position</strong>. This new position introduces two notes you have not played yet: <strong>A</strong> and <strong>B</strong>. Together with the C position, you will now know all seven natural notes of the musical alphabet.</p>

                <h3>Setting Up the G Position</h3>
                <p>Move your right hand up the keyboard so that your thumb (finger 1) is on <strong>G</strong>, the note you previously played with finger 5. Your new finger placement is:</p>
                <ul>
                    <li><strong>Finger 1</strong> (thumb) on <strong>G</strong></li>
                    <li><strong>Finger 2</strong> (index) on <strong>A</strong></li>
                    <li><strong>Finger 3</strong> (middle) on <strong>B</strong></li>
                    <li><strong>Finger 4</strong> (ring) on <strong>C</strong> (one octave above Middle C)</li>
                    <li><strong>Finger 5</strong> (pinky) on <strong>D</strong></li>
                </ul>
                <p>Notice that this G position overlaps with the C position, the note G appears in both. This overlap is the bridge between the two positions and will be important when you start moving between them.</p>

                <h3>The Complete Musical Alphabet</h3>
                <p>Between your C position and G position, you now have access to all seven natural notes: <strong>C, D, E, F, G, A, B</strong>. After B, the alphabet starts over at C (one octave higher). This repeating cycle of seven notes is the foundation of all Western music.</p>
                <p>Here is an important concept: the distance from one note to the very next note with the same name (for example, from C to the next C) is called an <strong>octave</strong>. You learned this in the first lesson. Now you can actually play it, place your thumb on Middle C, then find the next C up with your pinky in the G position. That span covers one full octave.</p>

                <h3>The G Pentascale</h3>
                <p>Just as you played a five-finger pattern starting on C, you can play one starting on G:</p>
                <p><strong>G → A → B → C → D → C → B → A → G</strong></p>
                <p>Play this ascending and descending several times. Compare the sound to the C pentascale. The G pentascale has a slightly different character because you start on a different pitch, but the intervallic pattern (the distances between notes) is the same.</p>

                <h3>Switching Between C and G Positions</h3>
                <p>A key skill to develop is moving smoothly between positions. Try this exercise:</p>
                <ol>
                    <li>Play the C pentascale up: <strong>C → D → E → F → G</strong></li>
                    <li>Shift your hand so your thumb moves from C to G.</li>
                    <li>Play the G pentascale up: <strong>G → A → B → C → D</strong></li>
                    <li>Shift back and play the C pentascale down: <strong>G → F → E → D → C</strong></li>
                </ol>
                <p>The shift should be smooth and quick, with minimal pause. Practice this transition until it feels natural.</p>

                <h3>Exercise 1: Name and Play All Seven Notes</h3>
                <p>Starting from Middle C, play each white key in order while saying the note name out loud: <strong>C, D, E, F, G, A, B, C</strong>. Then play back down: <strong>C, B, A, G, F, E, D, C</strong>. You will need to shift your hand position partway through. That is perfectly fine, just pause and reposition.</p>

                <h3>Exercise 2: G Position Melody</h3>
                <p>Play this simple melody in G position (all quarter notes):</p>
                <p><strong>G  A  B  A  |  G  G  A  A  |  B  B  C  C  |  D  (hold)</strong></p>
                <p>Count "1, 2, 3, 4" for each group of four. The last note (D) is a whole note, hold it for four full beats.</p>

                <h3>Why Multiple Positions Matter</h3>
                <p>Real piano music constantly moves across the keyboard. Knowing multiple positions is like knowing multiple words, the more positions you know, the more musical sentences you can play. Think of C position as your first word and G position as your second. Soon you will combine them fluently.</p>
                <p>Many famous melodies use both C and G positions. "Twinkle Twinkle Little Star" starts in C position, and the next module will show you how songs naturally flow between positions.</p>

                <h3>Recommended Practice Routine (12 minutes)</h3>
                <ol>
                    <li><strong>1 minute:</strong> Warm up with C pentascale (from previous module).</li>
                    <li><strong>3 minutes:</strong> G pentascale, ascending and descending, 10 times.</li>
                    <li><strong>3 minutes:</strong> Position switching exercise (C position to G position and back).</li>
                    <li><strong>3 minutes:</strong> Name and play all seven notes (Exercise 1).</li>
                    <li><strong>2 minutes:</strong> G position melody (Exercise 2).</li>
                </ol>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'In the G position (right hand), which finger plays the note A?',
                    'explanation' => 'In G position, finger 2 (index) plays A, since finger 1 (thumb) is placed on G.',
                    'options' => [
                        ['text' => 'Finger 1 (thumb)', 'correct' => false],
                        ['text' => 'Finger 2 (index)', 'correct' => true],
                        ['text' => 'Finger 3 (middle)', 'correct' => false],
                        ['text' => 'Finger 5 (pinky)', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Which note is shared between the C position and the G position?',
                    'explanation' => 'G is the top note of the C position (finger 5) and the bottom note of the G position (finger 1).',
                    'options' => [
                        ['text' => 'C', 'correct' => false],
                        ['text' => 'D', 'correct' => false],
                        ['text' => 'E', 'correct' => false],
                        ['text' => 'G', 'correct' => true],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'How many natural (white key) notes are in the musical alphabet?',
                    'explanation' => 'There are 7 natural notes (A, B, C, D, E, F, G) before the pattern repeats at a higher octave.',
                    'options' => [
                        ['text' => '5', 'correct' => false],
                        ['text' => '6', 'correct' => false],
                        ['text' => '7', 'correct' => true],
                        ['text' => '8', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You want to play a melody that uses both C and A in the right hand. Which position change strategy should you use?',
                    'explanation' => 'Since C position covers C-G and G position covers G-D, shifting from C to G position gives you access to both notes.',
                    'options' => [
                        ['text' => 'Stay in C position and stretch finger 5 to reach A', 'correct' => false],
                        ['text' => 'Shift from C position to G position when you reach G', 'correct' => true],
                        ['text' => 'Play A with the left hand instead', 'correct' => false],
                        ['text' => 'Skip the A note entirely', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 2: Introduction to Chords
        // ====================================================================
        [
            'title' => 'Introduction to Chords',
            'duration' => 15,
            'difficulty' => 3,
            'xp' => 70,
            'video' => '',
            'interactivity' => [
                'type' => 'ear_trainer',
                'target' => 5,
                'label' => 'Identify 5 chords (C, F, G major) by ear'
            ],
            'content' => '
                <h2>What Is a Chord?</h2>
                <p>So far, you have been playing one note at a time, this is called a <strong>melody</strong>. Now it is time to discover <strong>chords</strong>: multiple notes played at the same time. Chords add depth, richness, and harmony to music. They are the backbone of virtually every song you have ever heard.</p>
                <p>A <strong>chord</strong> is three or more notes played simultaneously. The most basic type of chord is a <strong>triad</strong>, which uses exactly three notes.</p>

                <h3>Building a Major Triad</h3>
                <p>A <strong>major triad</strong> is built by stacking two intervals on top of a starting note (called the <strong>root</strong>):</p>
                <ul>
                    <li>The <strong>root</strong>: the note the chord is named after.</li>
                    <li>The <strong>third</strong>: the note that is 4 half steps above the root (a major third).</li>
                    <li>The <strong>fifth</strong>: the note that is 7 half steps above the root (a perfect fifth).</li>
                </ul>
                <p>For now, the easiest way to think about it: take a note, skip one white key, add a note, skip one white key, add a note. This gives you a triad on the white keys.</p>

                <h3>Three Essential Chords: C, F, and G Major</h3>
                <p>Let us build the three most important chords for beginners:</p>
                <ul>
                    <li><strong>C major</strong>: <strong>C + E + G</strong> (fingers 1, 3, 5 in C position)</li>
                    <li><strong>F major</strong>: <strong>F + A + C</strong> (you will need to move your hand)</li>
                    <li><strong>G major</strong>: <strong>G + B + D</strong> (fingers 1, 3, 5 in G position)</li>
                </ul>
                <p>Play each chord by pressing all three notes at the same time. The three notes should sound as one unified sound, not three separate attacks. Press them simultaneously and listen to how full and rich a chord sounds compared to a single note.</p>

                <h3>The I-IV-V Progression</h3>
                <p>These three chords, C, F, and G, are called the <strong>primary chords</strong> of the key of C major. In music theory, they are numbered with Roman numerals based on which scale degree they start on:</p>
                <ul>
                    <li><strong>I</strong> (one) = C major (starts on the 1st note of the C scale)</li>
                    <li><strong>IV</strong> (four) = F major (starts on the 4th note)</li>
                    <li><strong>V</strong> (five) = G major (starts on the 5th note)</li>
                </ul>
                <p>The <strong>I-IV-V-I progression</strong> (C → F → G → C) is the foundation of thousands of songs across pop, rock, folk, classical, and more. Learning this progression is one of the most useful things you can do as a beginning pianist.</p>

                <h3>Exercise 1: Play Each Chord</h3>
                <p>Practice playing each chord separately. Press all three notes at exactly the same time:</p>
                <ol>
                    <li>Play <strong>C major</strong> (C-E-G) and hold for 4 beats.</li>
                    <li>Play <strong>F major</strong> (F-A-C) and hold for 4 beats.</li>
                    <li>Play <strong>G major</strong> (G-B-D) and hold for 4 beats.</li>
                </ol>
                <p>Repeat each chord 4 times, listening for all three notes sounding clearly and together.</p>

                <h3>Exercise 2: The I-IV-V-I Progression</h3>
                <p>Now play the chords in sequence, holding each for 4 beats:</p>
                <p><strong>C major (4 beats) → F major (4 beats) → G major (4 beats) → C major (4 beats)</strong></p>
                <p>This is the I-IV-V-I progression in the key of C. Repeat it several times. You are now playing the harmonic foundation that supports most popular music. Try counting "1, 2, 3, 4" on each chord to keep a steady pace.</p>

                <h3>Practice Tips</h3>
                <p>When switching between chords, minimize hand movement. For example, when moving from C major to F major, notice that the note C is in both chords, your finger 5 (pinky) on C can stay in roughly the same area. Look for these common notes between chords to make transitions smoother.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'How many notes make up a triad?',
                    'explanation' => 'A triad consists of exactly 3 notes: the root, the third, and the fifth.',
                    'options' => [
                        ['text' => '2 notes', 'correct' => false],
                        ['text' => '3 notes', 'correct' => true],
                        ['text' => '4 notes', 'correct' => false],
                        ['text' => '5 notes', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Which notes make up the C major chord?',
                    'explanation' => 'C major is built from C (root), E (third), and G (fifth).',
                    'options' => [
                        ['text' => 'C, D, E', 'correct' => false],
                        ['text' => 'C, F, G', 'correct' => false],
                        ['text' => 'C, E, G', 'correct' => true],
                        ['text' => 'C, E, A', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'In the key of C major, the IV chord is:',
                    'explanation' => 'The IV chord is built on the 4th note of the C major scale (F), so it is F major (F-A-C).',
                    'options' => [
                        ['text' => 'D major', 'correct' => false],
                        ['text' => 'E major', 'correct' => false],
                        ['text' => 'G major', 'correct' => false],
                        ['text' => 'F major', 'correct' => true],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'When playing the I-IV-V-I progression (C → F → G → C), you notice a "bump" between each chord. What is the best way to smooth out the transitions?',
                    'explanation' => 'Finding common notes between chords and minimizing hand movement creates smoother transitions.',
                    'options' => [
                        ['text' => 'Play each chord louder to cover the gap', 'correct' => false],
                        ['text' => 'Add a rest between each chord', 'correct' => false],
                        ['text' => 'Look for common notes between chords and minimize hand movement', 'correct' => true],
                        ['text' => 'Play the progression faster so the gaps are shorter', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 3: Eighth Notes and Faster Rhythms
        // ====================================================================
        [
            'title' => 'Eighth Notes and Rhythms',
            'duration' => 12,
            'difficulty' => 3,
            'xp' => 70,
            'video' => '',
            'interactivity' => null,
            'content' => '
                <h2>Picking Up the Pace: Eighth Notes</h2>
                <p>You already know whole notes (4 beats), half notes (2 beats), and quarter notes (1 beat). Now it is time to learn a note value that is twice as fast as a quarter note: the <strong>eighth note</strong>. Eighth notes add energy and movement to music, and they appear in almost every song you will play.</p>

                <h3>What Is an Eighth Note?</h3>
                <p>An <strong>eighth note</strong> lasts for half a beat. This means two eighth notes fit into the same time as one quarter note. If a quarter note gets one foot tap, an eighth note gets half a foot tap.</p>
                <p>The mathematical relationship is simple:</p>
                <ul>
                    <li>1 whole note = 2 half notes = 4 quarter notes = <strong>8 eighth notes</strong></li>
                    <li>1 quarter note = <strong>2 eighth notes</strong></li>
                </ul>

                <h3>Counting Eighth Notes</h3>
                <p>To count eighth notes, we add the word "and" between each beat number. This gives us a place for the second eighth note in each beat:</p>
                <p><strong>"1 - and - 2 - and - 3 - and - 4 - and"</strong></p>
                <p>The numbers (1, 2, 3, 4) fall on the <strong>downbeats</strong>. The "ands" fall on the <strong>upbeats</strong>. Together, they divide each beat exactly in half.</p>
                <p>Try this: tap your foot at a steady pace for quarter notes. Now clap twice for each foot tap. The foot hits on the number, and your second clap hits on the "and." That is the eighth-note rhythm.</p>

                <h3>Mixing Note Values</h3>
                <p>Most music uses a mix of different note values. Here is a common pattern that mixes quarter notes and eighth notes in 4/4 time:</p>
                <p><strong>Quarter - Quarter - Eighth - Eighth - Quarter</strong></p>
                <p>Counted out: <strong>"1, 2, 3 - and - 4"</strong></p>
                <p>The first two beats are quarter notes (one note per beat). Beat 3 splits into two eighth notes. Beat 4 is another quarter note.</p>

                <h3>The Quarter Rest</h3>
                <p>A <strong>quarter rest</strong> is one beat of silence. When you see a quarter rest, you do not play anything for that beat, but you keep counting. Rests give music space to breathe.</p>
                <p>Here is a pattern with a rest: <strong>Quarter - Quarter - Rest - Quarter</strong> (counted: "1, 2, 3, 4" with nothing played on beat 3).</p>

                <h3>Exercise 1: Eighth Note Clapping</h3>
                <p>Before playing on the piano, practice clapping these rhythms:</p>
                <ol>
                    <li>Eight eighth notes: clap on every "and", <strong>"1-and-2-and-3-and-4-and"</strong></li>
                    <li>Quarter, two eighths, quarter, two eighths: <strong>"1, 2-and, 3, 4-and"</strong></li>
                    <li>Two eighths, quarter, quarter, half: <strong>"1-and, 2, 3, 4 (hold)"</strong></li>
                </ol>

                <h3>Exercise 2: Eighth Notes on the Piano</h3>
                <p>Place your right hand in C position. Play the following, counting "1-and-2-and-3-and-4-and":</p>
                <ol>
                    <li>Play C as eight eighth notes (two per beat). Keep them very even.</li>
                    <li>Now play: <strong>C(quarter) - D(quarter) - E-E(two eighths) - F(quarter)</strong></li>
                    <li>Finally: <strong>C-D(two eighths) - E(quarter) - F-G(two eighths) - G(quarter)</strong></li>
                </ol>
                <p>Use a metronome at 60 BPM. Each click is one quarter note. Eighth notes should fall exactly between the clicks.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'How many eighth notes equal one quarter note?',
                    'explanation' => 'Two eighth notes fit in the same time as one quarter note, since each eighth note lasts half a beat.',
                    'options' => [
                        ['text' => '1 eighth note', 'correct' => false],
                        ['text' => '2 eighth notes', 'correct' => true],
                        ['text' => '4 eighth notes', 'correct' => false],
                        ['text' => '8 eighth notes', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'When counting eighth notes, what word do you say between the beat numbers?',
                    'explanation' => 'Eighth notes are counted as "1-and-2-and-3-and-4-and," with "and" marking the upbeat.',
                    'options' => [
                        ['text' => '"plus"', 'correct' => false],
                        ['text' => '"then"', 'correct' => false],
                        ['text' => '"and"', 'correct' => true],
                        ['text' => '"rest"', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'How many eighth notes fit in one measure of 4/4 time?',
                    'explanation' => 'In 4/4 time there are 4 beats. Each beat can hold 2 eighth notes, so 4 × 2 = 8 eighth notes total.',
                    'options' => [
                        ['text' => '4', 'correct' => false],
                        ['text' => '6', 'correct' => false],
                        ['text' => '8', 'correct' => true],
                        ['text' => '16', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You are playing a piece with a mix of quarter and eighth notes. The eighth notes always sound rushed. What should you do?',
                    'explanation' => 'A metronome helps ensure eighth notes are exactly half a beat and not rushed or uneven.',
                    'options' => [
                        ['text' => 'Play the whole piece faster so the difference is less noticeable', 'correct' => false],
                        ['text' => 'Use a metronome and count "1-and-2-and" out loud to ensure even subdivision', 'correct' => true],
                        ['text' => 'Replace the eighth notes with quarter notes', 'correct' => false],
                        ['text' => 'Lift your fingers higher before playing eighth notes', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 4: Playing Twinkle Twinkle Little Star
        // ====================================================================
        [
            'title' => 'Playing Twinkle Twinkle',
            'duration' => 15,
            'difficulty' => 3,
            'xp' => 70,
            'video' => '',
            'interactivity' => [
                'type' => 'piano_hero',
                'target' => 5,
                'label' => 'Play Twinkle Twinkle in Piano Hero for 5 minutes'
            ],
            'content' => '
                <h2>Your First Complete Song</h2>
                <p>This is a milestone lesson. You are about to play your first complete song from beginning to end: <strong>"Twinkle, Twinkle, Little Star."</strong> This beloved melody is perfect for beginners because it uses simple note values (quarter and half notes) and stays mostly within positions you already know.</p>

                <h3>The Melody</h3>
                <p>The melody of "Twinkle, Twinkle, Little Star" uses six different notes: <strong>C, D, E, F, G, and A</strong>. Since A is not in the basic C position, you will need to shift briefly to reach it. Here is the full melody, with each note name written out:</p>
                <p><strong>Line 1:</strong> C C G G | A A G (hold) | F F E E | D D C (hold)</p>
                <p><strong>Line 2:</strong> G G F F | E E D (hold) | G G F F | E E D (hold)</p>
                <p><strong>Line 3:</strong> C C G G | A A G (hold) | F F E E | D D C (hold)</p>
                <p>Each note without a dash is a quarter note (1 beat). A dash means the previous note is held for an extra beat (making it a half note, 2 beats). The whole song is in 4/4 time, so each group of notes between the vertical bars represents one measure of 4 beats.</p>

                <h3>Handling the Position Shift</h3>
                <p>The note <strong>A</strong> appears in Line 1. Since A is not in the standard C five-finger position, you have two options:</p>
                <ul>
                    <li><strong>Option 1:</strong> Briefly shift your hand up so finger 1 lands on G and finger 2 on A, then shift back.</li>
                    <li><strong>Option 2:</strong> Stretch finger 5 (or finger 4) slightly to reach A while keeping the rest of your hand near C position.</li>
                </ul>
                <p>At this stage, either approach is fine. Choose whichever feels more comfortable. As you gain experience, position shifts will become second nature.</p>

                <h3>Learning Strategy: Section by Section</h3>
                <p>Do not try to play the entire song at once. Break it into three sections:</p>
                <ol>
                    <li><strong>Section A</strong> (Line 1): C C G G A A G (hold) F F E E D D C (hold)</li>
                    <li><strong>Section B</strong> (Line 2): G G F F E E D (hold) (played twice)</li>
                    <li><strong>Section A again</strong> (Line 3): Same as Line 1</li>
                </ol>
                <p>Notice that Lines 1 and 3 are identical. This is called <strong>ABA form</strong>, a common structure in music where the first section returns at the end. You only need to learn two sections to play the whole song.</p>

                <h3>Exercise 1: Learn Section by Section</h3>
                <ol>
                    <li>Practice <strong>Section A</strong> alone, 5 times, very slowly with counting.</li>
                    <li>Practice <strong>Section B</strong> alone, 5 times.</li>
                    <li>Connect A and B: play Section A, then Section B without stopping.</li>
                    <li>Add the final Section A to play the complete song.</li>
                </ol>

                <h3>Exercise 2: Add Musical Expression</h3>
                <p>Once you can play the notes correctly, start making it sound more musical:</p>
                <ul>
                    <li>Play the first note of each measure slightly louder to emphasize the <strong>downbeat</strong>.</li>
                    <li>Try making Section A a little louder and Section B a little softer for contrast.</li>
                    <li>Connect the notes smoothly, do not lift your finger off one key until you press the next key.</li>
                </ul>
                <p>Use Piano Hero to practice this song with visual feedback. The falling notes will help you keep a steady rhythm.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What is the form (structure) of "Twinkle, Twinkle, Little Star"?',
                    'explanation' => 'The song has ABA form: Section A plays, then Section B, then Section A repeats identically.',
                    'options' => [
                        ['text' => 'ABC form', 'correct' => false],
                        ['text' => 'ABA form', 'correct' => true],
                        ['text' => 'AABB form', 'correct' => false],
                        ['text' => 'Through-composed (no repeats)', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'How many different notes does the melody of "Twinkle, Twinkle, Little Star" use?',
                    'explanation' => 'The melody uses six notes: C, D, E, F, G, and A.',
                    'options' => [
                        ['text' => '4 notes', 'correct' => false],
                        ['text' => '5 notes', 'correct' => false],
                        ['text' => '6 notes', 'correct' => true],
                        ['text' => '7 notes', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'In the first measure of "Twinkle" (C C G G), how many beats are there?',
                    'explanation' => 'Each note is a quarter note (1 beat), and there are 4 notes, making 4 beats total in 4/4 time.',
                    'options' => [
                        ['text' => '2 beats', 'correct' => false],
                        ['text' => '3 beats', 'correct' => false],
                        ['text' => '4 beats', 'correct' => true],
                        ['text' => '8 beats', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You can play each section of "Twinkle" separately but struggle to connect them without pausing. What is the best practice approach?',
                    'explanation' => 'Practicing the transition between sections specifically (last 2 measures of one section into first 2 of the next) targets the weak point directly.',
                    'options' => [
                        ['text' => 'Only practice the whole song from start to finish, repeatedly', 'correct' => false],
                        ['text' => 'Focus practice on the transition points, the last 2 measures of one section into the first 2 of the next', 'correct' => true],
                        ['text' => 'Play each section at a different tempo', 'correct' => false],
                        ['text' => 'Add a long pause between sections', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 5: Module Review and Ear Training Challenge
        // ====================================================================
        [
            'title' => 'Module Review and Challenge',
            'duration' => 15,
            'difficulty' => 4,
            'xp' => 80,
            'video' => '',
            'interactivity' => [
                'type' => 'ear_trainer',
                'target' => 10,
                'label' => 'Complete 10 Ear Trainer questions to finish this module'
            ],
            'content' => '
                <h2>Module 2 Review: Everything You Have Learned</h2>
                <p>Congratulations on reaching the end of Module 2. You have made remarkable progress. Let us review everything you have learned and make sure you are ready for the next step: reading music notation in Module 3.</p>

                <h3>Skills Checklist</h3>
                <p>By completing this module, you should be able to:</p>
                <ul>
                    <li>Play in both the <strong>C position</strong> and <strong>G position</strong> with your right hand.</li>
                    <li>Name and play all seven natural notes: <strong>C, D, E, F, G, A, B</strong>.</li>
                    <li>Shift smoothly between C and G positions.</li>
                    <li>Play <strong>C major, F major, and G major</strong> chords.</li>
                    <li>Play the <strong>I-IV-V-I progression</strong> in the key of C.</li>
                    <li>Count and play <strong>whole, half, quarter, and eighth notes</strong>.</li>
                    <li>Count eighth notes using the <strong>"1-and-2-and"</strong> method.</li>
                    <li>Understand and observe <strong>rests</strong>.</li>
                    <li>Play "Twinkle, Twinkle, Little Star" from beginning to end.</li>
                </ul>

                <h3>Review Exercise 1: Comprehensive Warm-Up</h3>
                <p>Play through these exercises as a warm-up routine:</p>
                <ol>
                    <li><strong>C pentascale</strong> up and down, 2 times.</li>
                    <li><strong>G pentascale</strong> up and down, 2 times.</li>
                    <li><strong>All seven notes</strong> ascending (C to B) and descending (B to C), with position shift.</li>
                    <li><strong>C major chord</strong>, held for 4 beats.</li>
                    <li><strong>F major chord</strong>, held for 4 beats.</li>
                    <li><strong>G major chord</strong>, held for 4 beats.</li>
                    <li><strong>I-IV-V-I progression</strong> (C → F → G → C), each chord 4 beats.</li>
                </ol>

                <h3>Review Exercise 2: Rhythm Patterns</h3>
                <p>Play the note C with these rhythm patterns, counting out loud:</p>
                <ol>
                    <li>4 quarter notes: "1, 2, 3, 4"</li>
                    <li>2 half notes: "1-2, 3-4"</li>
                    <li>1 whole note: "1-2-3-4"</li>
                    <li>8 eighth notes: "1-and-2-and-3-and-4-and"</li>
                    <li>Mix: quarter, two eighths, quarter, two eighths: "1, 2-and, 3, 4-and"</li>
                </ol>

                <h3>Review Exercise 3: Play Your Songs</h3>
                <p>Play through both songs you have learned:</p>
                <ol>
                    <li><strong>"Mary Had a Little Lamb"</strong> (from Module 1, Lesson 4)</li>
                    <li><strong>"Twinkle, Twinkle, Little Star"</strong> (from this module)</li>
                </ol>
                <p>For each song, focus on playing with a steady tempo, clear tone, and relaxed posture. If you own a recording device (your phone works perfectly), record yourself and listen back. You might be surprised at how good you already sound.</p>

                <h3>Ear Training Challenge</h3>
                <p>Complete the <strong>Ear Trainer</strong> challenge to finish this module. You will need to identify 10 notes by ear. The Ear Trainer will play a note, and you choose which note it is. Use what you have learned about the sound of each note to guide your choices. Trust your ears, they are more capable than you think.</p>

                <h3>What Comes Next</h3>
                <p>In <strong>Module 3: Reading Music</strong>, you will learn to read standard music notation. You will discover the staff, the treble and bass clefs, and how those dots on a page translate to the notes you have been playing. This is where your piano journey truly opens up, once you can read music, you can learn any song on your own.</p>
                <p>Great work completing Module 2. You are already a pianist. Keep going.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What three chords make up the I-IV-V progression in C major?',
                    'explanation' => 'In C major, I = C major, IV = F major, V = G major. These are the three primary chords.',
                    'options' => [
                        ['text' => 'C major, D major, E major', 'correct' => false],
                        ['text' => 'C major, F major, G major', 'correct' => true],
                        ['text' => 'A minor, D minor, E minor', 'correct' => false],
                        ['text' => 'C major, E major, A major', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Which note value is the shortest you have learned so far?',
                    'explanation' => 'The eighth note (half a beat) is the shortest note value covered so far. It is half the length of a quarter note.',
                    'options' => [
                        ['text' => 'Whole note', 'correct' => false],
                        ['text' => 'Half note', 'correct' => false],
                        ['text' => 'Quarter note', 'correct' => false],
                        ['text' => 'Eighth note', 'correct' => true],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The G position starts with which finger on which note?',
                    'explanation' => 'The G position places finger 1 (thumb) on G, with the other fingers on A, B, C, and D.',
                    'options' => [
                        ['text' => 'Finger 1 on C', 'correct' => false],
                        ['text' => 'Finger 5 on G', 'correct' => false],
                        ['text' => 'Finger 1 on G', 'correct' => true],
                        ['text' => 'Finger 3 on G', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'A friend asks you what skills they need before starting Module 3 (Reading Music). Which of the following is the best summary?',
                    'explanation' => 'Module 3 requires comfort with both hand positions, all 7 natural notes, basic chords, and rhythm fundamentals.',
                    'options' => [
                        ['text' => 'They only need to know Middle C', 'correct' => false],
                        ['text' => 'They need C and G positions, all 7 natural notes, basic chords (C, F, G), and quarter/eighth note rhythms', 'correct' => true],
                        ['text' => 'They need to be able to play with both hands together', 'correct' => false],
                        ['text' => 'They need to be able to read sheet music already', 'correct' => false],
                    ]
                ],
            ]
        ],

    ]; // end Module 2 lessons
}

function pm_get_seed_lessons_module3() {
    return [

        // ====================================================================
        // LESSON 1: The Staff and Clefs
        // ====================================================================
        [
            'title' => 'The Staff and Clefs',
            'duration' => 12,
            'difficulty' => 2,
            'xp' => 60,
            'video' => '',
            'interactivity' => [
                'type' => 'sight_reading',
                'target' => 5,
                'label' => 'Identify 5 notes on the staff'
            ],
            'content' => '
                <h2>Unlocking Written Music</h2>
                <p>Until now, you have been learning notes by their letter names and positions on the keyboard. That is a great start, but to play music independently, to pick up any piece of sheet music and bring it to life, you need to learn <strong>music notation</strong>. In this lesson, you will discover the framework on which all written music is built: the <strong>staff</strong>, the <strong>clefs</strong>, and the <strong>grand staff</strong>.</p>

                <h3>The Staff</h3>
                <p>Music is written on a set of five horizontal lines called a <strong>staff</strong> (sometimes called a "stave"). The staff contains five lines and four spaces between them. Notes are placed on these lines and spaces to indicate pitch, higher notes sit higher on the staff, and lower notes sit lower.</p>
                <p>The lines are numbered from bottom to top: the bottom line is line 1 and the top line is line 5. The spaces are also numbered from bottom to top: the bottom space is space 1 and the top space is space 4.</p>

                <h3>The Treble Clef</h3>
                <p>A <strong>clef</strong> is a symbol placed at the beginning of the staff that tells you which notes the lines and spaces represent. The <strong>treble clef</strong> (also called the <strong>G clef</strong>) is the most common clef. It looks like a fancy cursive letter and its inner loop wraps around the second line of the staff, which represents the note <strong>G</strong>.</p>
                <p>The treble clef is used for higher-pitched notes. On the piano, the treble clef generally covers the notes you play with your <strong>right hand</strong>, the notes at and above Middle C.</p>

                <h3>The Bass Clef</h3>
                <p>The <strong>bass clef</strong> (also called the <strong>F clef</strong>) covers lower-pitched notes. It looks like a backwards "C" with two dots, and those two dots surround the fourth line of the staff, which represents the note <strong>F</strong> (specifically, the F below Middle C).</p>
                <p>On the piano, the bass clef generally covers the notes you play with your <strong>left hand</strong>, the notes below Middle C.</p>

                <h3>The Grand Staff</h3>
                <p>Piano music uses both clefs at the same time. When the treble staff and bass staff are joined together by a vertical line and a <strong>brace</strong> (a curly bracket on the left side), the result is called the <strong>grand staff</strong>. The treble staff sits on top and the bass staff sits on the bottom.</p>
                <p>Right between the two staves, you will find <strong>Middle C</strong>. It sits on a short line of its own called a <strong>ledger line</strong>, a small extra line drawn above or below the staff for notes that do not fit on the five main lines. Middle C can be written just below the treble staff or just above the bass staff. Either way, it is the same note.</p>

                <h3>Barlines and Measures</h3>
                <p>Vertical lines called <strong>barlines</strong> divide the staff into sections called <strong>measures</strong> (also called "bars"). Each measure contains a specific number of beats, determined by the time signature (which you will learn in Lesson 4). Barlines help you keep your place and organize the music into manageable groups.</p>
                <p>At the end of a piece, you will see a <strong>double barline</strong> (two vertical lines, the second one thicker), which signals the end of the music.</p>

                <h3>Exercise 1: Identify the Parts</h3>
                <p>Open any piece of sheet music (you can use the scores on PianoMode\'s Listen and Play page). Find and identify:</p>
                <ol>
                    <li>The treble clef (curly symbol at the start of the top staff)</li>
                    <li>The bass clef (symbol with two dots at the start of the bottom staff)</li>
                    <li>The brace connecting them (curly bracket on the left)</li>
                    <li>Barlines dividing the music into measures</li>
                    <li>The double barline at the end</li>
                </ol>

                <h3>Exercise 2: Find Middle C</h3>
                <p>On your keyboard, play Middle C. Now look at a piece of sheet music and find every note that sits on a ledger line between the two staves. That is Middle C, the bridge between your right hand territory (treble) and left hand territory (bass).</p>

                <h3>Why Learning to Read Music is Worth the Effort</h3>
                <p>Many beginners wonder if they really need to read music. After all, you can learn songs by watching videos or following note names. But reading music gives you a superpower: <strong>independence</strong>. With music reading skills, you can:</p>
                <ul>
                    <li>Learn any song on your own, without needing a tutorial video.</li>
                    <li>Communicate with other musicians using a universal language.</li>
                    <li>Access hundreds of years of classical, jazz, and popular music.</li>
                    <li>Understand the structure and theory behind the music you play.</li>
                </ul>
                <p>Think of sheet music as a detailed map. You could explore a city without a map, but with one, you can go anywhere confidently and discover places you never knew existed.</p>

                <h3>A Visual Summary</h3>
                <p>Here is how the grand staff maps to the piano:</p>
                <ul>
                    <li><strong>Bass clef (bottom staff):</strong> Notes your left hand plays (below Middle C).</li>
                    <li><strong>Middle C (ledger line):</strong> The bridge note between both hands.</li>
                    <li><strong>Treble clef (top staff):</strong> Notes your right hand plays (at and above Middle C).</li>
                </ul>
                <p>In the next lesson, you will learn exactly which notes sit on which lines and spaces of the treble clef.</p>

                <h3>Recommended Practice Routine (10 minutes)</h3>
                <ol>
                    <li><strong>3 minutes:</strong> Find and identify all parts of the grand staff on a piece of sheet music (Exercise 1).</li>
                    <li><strong>3 minutes:</strong> Find every Middle C in the first page of a score.</li>
                    <li><strong>4 minutes:</strong> Play Middle C on your piano, then play notes above it (right hand) and below it (left hand). Connect what you see on paper to what you hear and feel on the keyboard.</li>
                </ol>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'How many lines does a musical staff have?',
                    'explanation' => 'A standard musical staff consists of 5 horizontal lines with 4 spaces between them.',
                    'options' => [
                        ['text' => '3 lines', 'correct' => false],
                        ['text' => '4 lines', 'correct' => false],
                        ['text' => '5 lines', 'correct' => true],
                        ['text' => '6 lines', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The treble clef is also known as the:',
                    'explanation' => 'The treble clef is called the G clef because its inner loop circles the second line, which represents G.',
                    'options' => [
                        ['text' => 'C clef', 'correct' => false],
                        ['text' => 'F clef', 'correct' => false],
                        ['text' => 'G clef', 'correct' => true],
                        ['text' => 'D clef', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Where is Middle C located on the grand staff?',
                    'explanation' => 'Middle C sits on a ledger line between the treble and bass staves, connecting the two clefs.',
                    'options' => [
                        ['text' => 'On the top line of the treble staff', 'correct' => false],
                        ['text' => 'On the bottom line of the bass staff', 'correct' => false],
                        ['text' => 'On a ledger line between the treble and bass staves', 'correct' => true],
                        ['text' => 'On the third line of the treble staff', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You open a piano score and see two staves connected by a brace on the left. What is this combined structure called?',
                    'explanation' => 'The grand staff combines the treble and bass staves with a brace, allowing notation of the full piano range.',
                    'options' => [
                        ['text' => 'A double staff', 'correct' => false],
                        ['text' => 'The grand staff', 'correct' => true],
                        ['text' => 'The extended staff', 'correct' => false],
                        ['text' => 'A parallel staff', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 2: Notes on the Treble Clef
        // ====================================================================
        [
            'title' => 'Notes on the Treble Clef',
            'duration' => 12,
            'difficulty' => 2,
            'xp' => 60,
            'video' => '',
            'interactivity' => [
                'type' => 'sight_reading',
                'target' => 10,
                'label' => 'Identify 10 treble clef notes'
            ],
            'content' => '
                <h2>Reading the Treble Clef</h2>
                <p>Now that you know what a staff and clef look like, it is time to learn which notes live where on the treble clef. This is the clef you will read most often with your right hand. By the end of this lesson, you will be able to look at a note on the treble staff and immediately name it.</p>

                <h3>The Line Notes: E-G-B-D-F</h3>
                <p>The five lines of the treble staff, from bottom to top, represent these notes:</p>
                <ul>
                    <li>Line 1 (bottom): <strong>E</strong></li>
                    <li>Line 2: <strong>G</strong></li>
                    <li>Line 3: <strong>B</strong></li>
                    <li>Line 4: <strong>D</strong></li>
                    <li>Line 5 (top): <strong>F</strong></li>
                </ul>
                <p>A classic memory aid for these line notes is the sentence: <strong>"Every Good Boy Does Fine"</strong> (E-G-B-D-F). Each first letter matches a note, from bottom to top.</p>

                <h3>The Space Notes: F-A-C-E</h3>
                <p>The four spaces of the treble staff, from bottom to top, represent:</p>
                <ul>
                    <li>Space 1 (bottom): <strong>F</strong></li>
                    <li>Space 2: <strong>A</strong></li>
                    <li>Space 3: <strong>C</strong></li>
                    <li>Space 4 (top): <strong>E</strong></li>
                </ul>
                <p>The memory aid here is simple: the spaces spell the word <strong>FACE</strong>, from bottom to top.</p>

                <h3>Middle C on the Treble Staff</h3>
                <p>Remember that <strong>Middle C</strong> sits on a ledger line just below the treble staff. It is one step below the bottom line (E). So reading downward from line 1: E (line 1), D (space below line 1), C (ledger line below the staff). This is your anchor point, the note you already know from your very first lesson.</p>

                <h3>Reading Notes Step by Step</h3>
                <p>When you see a note on the staff, here is how to identify it quickly:</p>
                <ol>
                    <li>Determine if the note is on a <strong>line</strong> (the line passes through the middle of the note head) or in a <strong>space</strong> (the note head sits between two lines).</li>
                    <li>Count from a known note. The second line is G (circled by the treble clef). Work up or down from there, alternating line-space-line-space through the alphabet.</li>
                    <li>With practice, you will recognize most notes instantly without counting.</li>
                </ol>

                <h3>Exercise 1: Flash Card Drill</h3>
                <p>Use the <strong>Note Invaders</strong> app on PianoMode to practice recognizing treble clef notes. Set it to treble clef only. Try to identify as many notes as you can in 3 minutes. Your goal is to respond within 2 seconds per note.</p>

                <h3>Exercise 2: Play What You Read</h3>
                <p>Open the score for "The First Term at the Piano" on PianoMode\'s Listen and Play page. Look at the treble clef notes only. For each note, say its name out loud, then play it on the keyboard. Start with just the first 4 measures. Work slowly, accuracy matters more than speed right now.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What do the spaces of the treble clef spell from bottom to top?',
                    'explanation' => 'The four treble clef spaces spell FACE: F (bottom), A, C, E (top).',
                    'options' => [
                        ['text' => 'CAGE', 'correct' => false],
                        ['text' => 'FACE', 'correct' => true],
                        ['text' => 'BEAD', 'correct' => false],
                        ['text' => 'FADE', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Which note sits on the second line of the treble staff?',
                    'explanation' => 'The second line is G, which is why the treble clef is also called the G clef, it curls around this line.',
                    'options' => [
                        ['text' => 'E', 'correct' => false],
                        ['text' => 'F', 'correct' => false],
                        ['text' => 'G', 'correct' => true],
                        ['text' => 'A', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The mnemonic "Every Good Boy Does Fine" helps you remember:',
                    'explanation' => 'EGBDF are the line notes of the treble clef, from the bottom line to the top line.',
                    'options' => [
                        ['text' => 'The space notes of the treble clef', 'correct' => false],
                        ['text' => 'The line notes of the bass clef', 'correct' => false],
                        ['text' => 'The line notes of the treble clef', 'correct' => true],
                        ['text' => 'The notes of the C major scale', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You see a note sitting in the space directly above the bottom line of the treble staff. What note is it?',
                    'explanation' => 'The bottom line is E, so the first space above it is F. Moving up the treble staff alternates line-space through the alphabet.',
                    'options' => [
                        ['text' => 'D', 'correct' => false],
                        ['text' => 'E', 'correct' => false],
                        ['text' => 'F', 'correct' => true],
                        ['text' => 'G', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 3: Notes on the Bass Clef
        // ====================================================================
        [
            'title' => 'Notes on the Bass Clef',
            'duration' => 12,
            'difficulty' => 3,
            'xp' => 60,
            'video' => '',
            'interactivity' => [
                'type' => 'sight_reading',
                'target' => 10,
                'label' => 'Identify 10 bass clef notes'
            ],
            'content' => '
                <h2>Reading the Bass Clef</h2>
                <p>The bass clef covers the lower notes on the piano, the notes you will play primarily with your left hand. Learning to read bass clef notes is just as important as reading treble clef, because most piano music uses both staves simultaneously. In this lesson, you will memorize the bass clef lines and spaces.</p>

                <h3>The Line Notes: G-B-D-F-A</h3>
                <p>The five lines of the bass staff, from bottom to top, represent:</p>
                <ul>
                    <li>Line 1 (bottom): <strong>G</strong></li>
                    <li>Line 2: <strong>B</strong></li>
                    <li>Line 3: <strong>D</strong></li>
                    <li>Line 4: <strong>F</strong></li>
                    <li>Line 5 (top): <strong>A</strong></li>
                </ul>
                <p>A classic mnemonic for the bass clef lines is: <strong>"Good Boys Do Fine Always"</strong> (G-B-D-F-A). Another popular one is "Great Big Dogs Fight Animals."</p>

                <h3>The Space Notes: A-C-E-G</h3>
                <p>The four spaces of the bass staff, from bottom to top, represent:</p>
                <ul>
                    <li>Space 1 (bottom): <strong>A</strong></li>
                    <li>Space 2: <strong>C</strong></li>
                    <li>Space 3: <strong>E</strong></li>
                    <li>Space 4 (top): <strong>G</strong></li>
                </ul>
                <p>The mnemonic for the bass clef spaces is: <strong>"All Cows Eat Grass"</strong> (A-C-E-G).</p>

                <h3>Middle C on the Bass Staff</h3>
                <p>Just as Middle C sits on a ledger line below the treble staff, it also sits on a ledger line <strong>above</strong> the bass staff. It is one step above the top line (A). Reading upward from line 5: A (line 5), B (space above line 5), C (ledger line above the staff). This is the same Middle C, just written from the bass clef perspective.</p>

                <h3>Connecting Treble and Bass</h3>
                <p>Here is the beautiful logic of the grand staff: the treble clef reads down to Middle C, and the bass clef reads up to Middle C. They meet in the middle, covering the full range of the piano without any gaps. As you become comfortable with both clefs, you will begin to read them simultaneously, one eye on each staff.</p>

                <h3>Exercise 1: Ledger Lines Game</h3>
                <p>Use the <strong>Ledger Lines</strong> app on PianoMode to practice identifying notes that sit above and below the standard staff lines. Start with just the bass clef. Try to achieve 80% accuracy in a 3-minute session.</p>

                <h3>Exercise 2: Bass Clef Note Hunt</h3>
                <p>Open the score for "Ode to Joy" on PianoMode. Look at the bass clef (bottom staff) and identify every note in the first 8 measures. Write down the note names on a piece of paper, then check yourself by playing each note on the keyboard. How many did you get right?</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What do the bass clef space notes spell using the mnemonic?',
                    'explanation' => '"All Cows Eat Grass" represents the bass clef spaces: A, C, E, G from bottom to top.',
                    'options' => [
                        ['text' => 'FACE', 'correct' => false],
                        ['text' => 'ACEG (All Cows Eat Grass)', 'correct' => true],
                        ['text' => 'BEAD', 'correct' => false],
                        ['text' => 'CAGE', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Which note sits on the fourth line of the bass staff?',
                    'explanation' => 'The fourth line of the bass staff is F. The bass clef is also called the F clef because its two dots surround this line.',
                    'options' => [
                        ['text' => 'D', 'correct' => false],
                        ['text' => 'E', 'correct' => false],
                        ['text' => 'F', 'correct' => true],
                        ['text' => 'G', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'On the grand staff, Middle C appears on a ledger line:',
                    'explanation' => 'Middle C can be written on a ledger line below the treble staff or above the bass staff, it is the same note either way.',
                    'options' => [
                        ['text' => 'Only below the treble staff', 'correct' => false],
                        ['text' => 'Only above the bass staff', 'correct' => false],
                        ['text' => 'Either below the treble staff or above the bass staff', 'correct' => true],
                        ['text' => 'On the middle line of the treble staff', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You see a note on the bottom line of the bass staff. Using "Good Boys Do Fine Always," what note is it?',
                    'explanation' => 'The first letter of "Good Boys Do Fine Always" is G. The bottom line of the bass clef is G.',
                    'options' => [
                        ['text' => 'A', 'correct' => false],
                        ['text' => 'B', 'correct' => false],
                        ['text' => 'F', 'correct' => false],
                        ['text' => 'G', 'correct' => true],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 4: Time Signatures Explained
        // ====================================================================
        [
            'title' => 'Time Signatures Explained',
            'duration' => 15,
            'difficulty' => 3,
            'xp' => 70,
            'video' => '',
            'interactivity' => null,
            'content' => '
                <h2>The Rhythm Roadmap: Time Signatures</h2>
                <p>You already know about note values, whole, half, quarter, and eighth notes. But how do you know how many beats fit in each measure? That is the job of the <strong>time signature</strong>. In this lesson, you will learn to read the three most common time signatures and understand what they mean for your playing.</p>

                <h3>What Is a Time Signature?</h3>
                <p>A <strong>time signature</strong> appears at the very beginning of a piece, right after the clef. It looks like two numbers stacked on top of each other (like a fraction without the line). Each number tells you something specific:</p>
                <ul>
                    <li>The <strong>top number</strong> tells you how many beats are in each measure.</li>
                    <li>The <strong>bottom number</strong> tells you which note value gets one beat.</li>
                </ul>
                <p>When the bottom number is 4, a quarter note gets one beat. When the bottom number is 8, an eighth note gets one beat.</p>

                <h3>4/4 Time (Common Time)</h3>
                <p><strong>4/4 time</strong> is the most common time signature in Western music. The top number (4) means there are 4 beats per measure. The bottom number (4) means a quarter note gets one beat.</p>
                <p>This means each measure holds the equivalent of 4 quarter notes. You could also fill a measure with 2 half notes, 1 whole note, 8 eighth notes, or any combination that adds up to 4 beats. You will sometimes see 4/4 written as a large <strong>C</strong> symbol, which stands for "common time."</p>
                <p>Count a measure of 4/4: <strong>"1, 2, 3, 4"</strong> with a slight emphasis on beat 1 (the <strong>downbeat</strong>).</p>

                <h3>3/4 Time (Waltz Time)</h3>
                <p><strong>3/4 time</strong> has 3 beats per measure, and a quarter note still gets one beat. This creates a lilting, dance-like feel, it is the time signature of the waltz. The emphasis pattern is: <strong>STRONG - weak - weak</strong>.</p>
                <p>Count a measure of 3/4: <strong>"1, 2, 3"</strong> with emphasis on beat 1. Many famous pieces use 3/4 time, including waltzes by Strauss and Chopin, and even "Happy Birthday."</p>

                <h3>2/4 Time</h3>
                <p><strong>2/4 time</strong> has 2 beats per measure, with a quarter note getting one beat. It creates a strong, march-like feel: <strong>STRONG - weak, STRONG - weak</strong>. Marches, polkas, and some folk music use 2/4 time.</p>
                <p>Count a measure of 2/4: <strong>"1, 2"</strong> with emphasis on beat 1.</p>

                <h3>Feeling the Difference</h3>
                <p>The difference between these time signatures is not just mathematical, it changes how the music <strong>feels</strong>:</p>
                <ul>
                    <li><strong>4/4</strong> feels steady and balanced, like walking.</li>
                    <li><strong>3/4</strong> feels flowing and circular, like swaying or dancing.</li>
                    <li><strong>2/4</strong> feels driving and energetic, like marching.</li>
                </ul>

                <h3>Exercise 1: Clap in Different Time Signatures</h3>
                <ol>
                    <li>Clap in 4/4: accent beat 1, <strong>CLAP</strong> clap clap clap | <strong>CLAP</strong> clap clap clap</li>
                    <li>Clap in 3/4: accent beat 1, <strong>CLAP</strong> clap clap | <strong>CLAP</strong> clap clap</li>
                    <li>Clap in 2/4: accent beat 1, <strong>CLAP</strong> clap | <strong>CLAP</strong> clap</li>
                </ol>
                <p>Feel how different each one is, even though you are doing the same action.</p>

                <h3>Exercise 2: Identify the Time Signature</h3>
                <p>Open three different scores on PianoMode\'s Listen and Play page. Find the time signature at the beginning of each piece. Is it 4/4, 3/4, or something else? "Ode to Joy" is in 4/4. Look at the Trois Gymnopedies, what time signature do you see?</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'In a 4/4 time signature, the top number tells you:',
                    'explanation' => 'The top number indicates 4 beats per measure. The bottom number tells you which note value equals one beat.',
                    'options' => [
                        ['text' => 'There are 4 measures in the piece', 'correct' => false],
                        ['text' => 'A quarter note gets 4 beats', 'correct' => false],
                        ['text' => 'There are 4 beats in each measure', 'correct' => true],
                        ['text' => 'You should play at 4 beats per second', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Which time signature is commonly called "waltz time"?',
                    'explanation' => '3/4 time has 3 beats per measure with a strong-weak-weak pattern, creating the characteristic waltz feel.',
                    'options' => [
                        ['text' => '2/4', 'correct' => false],
                        ['text' => '3/4', 'correct' => true],
                        ['text' => '4/4', 'correct' => false],
                        ['text' => '6/8', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'In 3/4 time, how many quarter notes fit in one measure?',
                    'explanation' => 'The top number (3) tells you there are 3 beats per measure, so 3 quarter notes fill one complete measure.',
                    'options' => [
                        ['text' => '2', 'correct' => false],
                        ['text' => '3', 'correct' => true],
                        ['text' => '4', 'correct' => false],
                        ['text' => '6', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You are listening to a piece and notice the emphasis pattern is STRONG-weak, STRONG-weak, repeating. Which time signature is most likely?',
                    'explanation' => 'A pattern of two beats (strong-weak) repeating suggests 2/4 time, typical of marches and polkas.',
                    'options' => [
                        ['text' => '4/4', 'correct' => false],
                        ['text' => '3/4', 'correct' => false],
                        ['text' => '2/4', 'correct' => true],
                        ['text' => '6/8', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 5: Reading Your First Score
        // ====================================================================
        [
            'title' => 'Reading Your First Score',
            'duration' => 15,
            'difficulty' => 4,
            'xp' => 80,
            'video' => '',
            'interactivity' => [
                'type' => 'sight_reading',
                'target' => 10,
                'label' => 'Read and identify 10 notes on the grand staff'
            ],
            'content' => '
                <h2>Putting It All Together</h2>
                <p>You now know the treble clef notes, the bass clef notes, and how time signatures work. It is time to put all of these skills together and <strong>read your first complete score</strong>. In this lesson, you will learn a systematic approach for reading any piece of sheet music.</p>

                <h3>Before You Play: Score Analysis</h3>
                <p>Before playing a single note, take 30 seconds to examine the score. This quick analysis saves time and prevents mistakes:</p>
                <ol>
                    <li><strong>Time signature:</strong> How many beats per measure? (Top number.)</li>
                    <li><strong>Key signature:</strong> Are there any sharps or flats after the clef? For now, look for pieces with no sharps or flats (the key of C major).</li>
                    <li><strong>Starting note:</strong> What is the first note in the treble clef? In the bass clef?</li>
                    <li><strong>Range:</strong> What are the highest and lowest notes? Will you need to shift your hand position?</li>
                    <li><strong>Repeat signs:</strong> Are there any repeat marks or endings?</li>
                </ol>

                <h3>Reading Note by Note</h3>
                <p>When reading a new piece for the first time, go extremely slowly. Here is the process for each note:</p>
                <ol>
                    <li>Look at the note on the staff and identify its <strong>name</strong> (use your mnemonics).</li>
                    <li>Identify its <strong>duration</strong> (whole, half, quarter, or eighth note).</li>
                    <li>Play the correct key for the correct number of beats.</li>
                    <li>Move to the next note.</li>
                </ol>
                <p>Do not try to read ahead yet. Focus on one note at a time until the process becomes more automatic.</p>

                <h3>Connecting Notes to Keys</h3>
                <p>The biggest challenge for new readers is connecting what they see on the page to the physical keyboard. Here are some tips:</p>
                <ul>
                    <li>When notes <strong>step up</strong> on the staff (moving from a line to the next space, or space to the next line), your finger moves to the next white key to the right.</li>
                    <li>When notes <strong>step down</strong>, your finger moves one key to the left.</li>
                    <li>When notes <strong>skip</strong> (jumping from a line to the next line, or space to the next space), your finger skips one key.</li>
                </ul>
                <p>This <strong>intervallic reading</strong> approach, reading the distance between notes rather than naming every single note, is faster and will serve you well as you advance.</p>

                <h3>Exercise 1: Read a Simple Melody</h3>
                <p>Open "The First Term at the Piano" on PianoMode\'s Listen and Play page. Choose the first piece in the collection. Before playing:</p>
                <ol>
                    <li>Check the time signature.</li>
                    <li>Identify the first 4 notes by name.</li>
                    <li>Play those 4 notes slowly.</li>
                    <li>Continue adding 4 notes at a time until you can play the first line.</li>
                </ol>

                <h3>Exercise 2: Note Naming Speed Drill</h3>
                <p>Go to the <strong>Sight Reading</strong> app on PianoMode. Set it to display notes on the grand staff. For each note shown, name it and play it on your keyboard. Try to get through 20 notes in under 2 minutes.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What is the first thing you should check when looking at a new piece of music?',
                    'explanation' => 'Checking the time signature first tells you the rhythmic framework for the entire piece.',
                    'options' => [
                        ['text' => 'The last note', 'correct' => false],
                        ['text' => 'The time signature', 'correct' => true],
                        ['text' => 'The speed', 'correct' => false],
                        ['text' => 'The composer name', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'When a note moves from a line to the very next space above it, the interval is called a:',
                    'explanation' => 'Moving from a line to the adjacent space (or vice versa) is a step, the smallest movement in standard notation.',
                    'options' => [
                        ['text' => 'Skip', 'correct' => false],
                        ['text' => 'Step', 'correct' => true],
                        ['text' => 'Leap', 'correct' => false],
                        ['text' => 'Octave', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What does "intervallic reading" mean?',
                    'explanation' => 'Intervallic reading focuses on the distance between consecutive notes rather than naming each note individually.',
                    'options' => [
                        ['text' => 'Reading only the treble clef', 'correct' => false],
                        ['text' => 'Reading the distances between notes rather than naming each one', 'correct' => true],
                        ['text' => 'Reading at a specific tempo', 'correct' => false],
                        ['text' => 'Reading with both hands at once', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You see two consecutive notes: one on the third line and the next on the fourth line of the treble staff. What kind of interval is this?',
                    'explanation' => 'From one line to the next line (skipping the space between) is a skip (a third). Line-to-adjacent-space is a step; line-to-next-line is a skip.',
                    'options' => [
                        ['text' => 'A step (moving to the adjacent note)', 'correct' => false],
                        ['text' => 'A skip (jumping over one note)', 'correct' => true],
                        ['text' => 'A repeat (same note)', 'correct' => false],
                        ['text' => 'An octave', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 6: Sight Reading Practice
        // ====================================================================
        [
            'title' => 'Sight Reading Practice',
            'duration' => 15,
            'difficulty' => 4,
            'xp' => 80,
            'video' => '',
            'interactivity' => [
                'type' => 'sight_reading',
                'target' => 15,
                'label' => 'Complete 15 sight reading exercises'
            ],
            'content' => '
                <h2>Building Your Sight Reading Skills</h2>
                <p><strong>Sight reading</strong> is the ability to play a piece of music you have never seen before, reading it in real time from the written page. It is one of the most valuable skills a pianist can develop. In this lesson, you will learn techniques to improve your sight reading and put them into practice.</p>

                <h3>The Golden Rules of Sight Reading</h3>
                <ul>
                    <li><strong>Keep going:</strong> The most important rule. If you make a mistake, do not stop and go back. Keep moving forward. In real music-making, the music does not wait for you.</li>
                    <li><strong>Look ahead:</strong> While playing the current note, your eyes should already be looking at the next note (or even two notes ahead). This is called <strong>"eyes ahead"</strong> technique.</li>
                    <li><strong>Choose a slow tempo:</strong> Pick a tempo where you can read without stopping. It is better to play slowly and continuously than quickly with pauses.</li>
                    <li><strong>Recognize patterns:</strong> Look for familiar shapes, scales, arpeggios, repeated notes, or sequences you have already learned. These patterns allow you to read groups of notes instead of individual notes.</li>
                </ul>

                <h3>Steps vs. Skips vs. Leaps</h3>
                <p>When sight reading, categorize the distance between consecutive notes:</p>
                <ul>
                    <li><strong>Step:</strong> The note moves to the very next line or space. Your finger moves to the adjacent key. (Example: E to F, or G to A.)</li>
                    <li><strong>Skip:</strong> The note jumps over one line or space. Your finger skips one key. (Example: E to G, or F to A.) These are thirds.</li>
                    <li><strong>Leap:</strong> The note jumps over two or more lines or spaces. Your hand may need to shift position. (Example: C to G, a fifth.)</li>
                </ul>

                <h3>The Pre-Reading Scan</h3>
                <p>Before you start playing, take 10–15 seconds to scan the piece:</p>
                <ol>
                    <li>Check the <strong>time signature</strong>.</li>
                    <li>Look for the <strong>highest and lowest notes</strong> to plan your hand position.</li>
                    <li>Identify any <strong>tricky spots</strong>, big leaps, unusual rhythms, or position changes.</li>
                    <li>Set a <strong>slow, comfortable tempo</strong> in your mind before starting.</li>
                </ol>

                <h3>Exercise 1: Sight Reading Challenge</h3>
                <p>Use the <strong>Sight Reading</strong> app on PianoMode. Set the difficulty to beginner. For each exercise:</p>
                <ol>
                    <li>Do a quick 5-second scan.</li>
                    <li>Start playing at a slow tempo.</li>
                    <li>Do NOT stop or restart, keep moving forward.</li>
                    <li>After finishing, review which notes you missed.</li>
                </ol>
                <p>Complete at least 5 exercises. Your goal is not perfection, it is to keep a steady pulse while reading.</p>

                <h3>Exercise 2: Read from a Real Score</h3>
                <p>Open "12 Melodious and Very Easy Studies, Op. 63" on PianoMode\'s Listen and Play page. Choose the first study. Play only the right hand part (treble clef), sight reading it from beginning to end without stopping. Then try the left hand part (bass clef) separately.</p>

                <h3>Daily Sight Reading Habit</h3>
                <p>The single best thing you can do for your sight reading is to practice it <strong>every day</strong>, even for just 5 minutes. Use the Sight Reading app or open a new piece from the Listen and Play library. Over time, your brain will get faster at converting the visual information on the page into finger movements on the keyboard. This is a skill that improves with consistent, daily practice more than with occasional long sessions.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What is the most important rule of sight reading?',
                    'explanation' => 'In sight reading, maintaining the pulse and keeping forward momentum is more important than playing every note perfectly.',
                    'options' => [
                        ['text' => 'Play every note perfectly', 'correct' => false],
                        ['text' => 'Play as fast as possible', 'correct' => false],
                        ['text' => 'Keep going and do not stop for mistakes', 'correct' => true],
                        ['text' => 'Memorize the piece before playing', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What does "eyes ahead" mean in sight reading?',
                    'explanation' => 'Eyes ahead means reading one or two notes beyond what you are currently playing, so your fingers are always prepared.',
                    'options' => [
                        ['text' => 'Looking at the end of the piece first', 'correct' => false],
                        ['text' => 'Reading ahead of the note you are currently playing', 'correct' => true],
                        ['text' => 'Closing your eyes while playing', 'correct' => false],
                        ['text' => 'Looking at the keyboard instead of the page', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'When two consecutive notes jump from one line directly to the next line (skipping the space between), this is called a:',
                    'explanation' => 'A skip moves from line to next line (or space to next space), covering the distance of a third.',
                    'options' => [
                        ['text' => 'Step', 'correct' => false],
                        ['text' => 'Skip', 'correct' => true],
                        ['text' => 'Octave', 'correct' => false],
                        ['text' => 'Unison', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You want to improve your sight reading. What is the most effective practice approach?',
                    'explanation' => 'Short daily practice (even 5 minutes) builds sight reading ability faster than occasional long sessions.',
                    'options' => [
                        ['text' => 'One long 60-minute session per week', 'correct' => false],
                        ['text' => '5 minutes of daily sight reading with new material each time', 'correct' => true],
                        ['text' => 'Memorizing every piece before playing it', 'correct' => false],
                        ['text' => 'Only playing pieces you already know', 'correct' => false],
                    ]
                ],
            ]
        ],

    ]; // end Module 3 lessons
}


// ============================================================================
// MODULE 4: TWO HANDS TOGETHER (7 Lessons)
// ============================================================================

function pm_get_seed_lessons_module4() {
    return [

        [
            'title' => 'Left Hand C Position',
            'duration' => 12,
            'difficulty' => 2,
            'xp' => 60,
            'video' => '',
            'interactivity' => null,
            'content' => '
                <h2>Introducing Your Left Hand</h2>
                <p>Until now, your left hand has been resting while your right hand did all the work. That changes today. In this lesson, you will learn the <strong>left hand C position</strong>, which mirrors the right hand position you already know.</p>

                <h3>Left Hand Finger Placement</h3>
                <p>Place your left hand so that your <strong>pinky (finger 5)</strong> is on the C below Middle C. The fingering is a mirror image of the right hand:</p>
                <ul>
                    <li><strong>Finger 5</strong> (pinky) on <strong>C</strong></li>
                    <li><strong>Finger 4</strong> (ring) on <strong>D</strong></li>
                    <li><strong>Finger 3</strong> (middle) on <strong>E</strong></li>
                    <li><strong>Finger 2</strong> (index) on <strong>F</strong></li>
                    <li><strong>Finger 1</strong> (thumb) on <strong>G</strong></li>
                </ul>
                <p>Notice that the thumb is on the highest note (G) and the pinky is on the lowest (C). This is the opposite of the right hand, where the thumb plays the lowest note.</p>

                <h3>Left Hand Pentascale</h3>
                <p>Play the C pentascale with your left hand: <strong>C → D → E → F → G → F → E → D → C</strong> (fingers 5, 4, 3, 2, 1, 2, 3, 4, 5). Keep the same curved hand shape and fingertip contact you use with your right hand.</p>
                <p>The left hand may feel clumsy at first, this is completely normal. Your non-dominant hand simply needs time to develop the same coordination as your dominant hand.</p>

                <h3>Common Left Hand Challenges</h3>
                <ul>
                    <li><strong>Finger 4 and 5 weakness:</strong> The ring finger and pinky of the left hand are typically the weakest fingers. Play each one slowly and firmly, listening for clear tone.</li>
                    <li><strong>Uneven volume:</strong> The thumb (finger 1) tends to play too loudly because it is the strongest finger. Consciously lighten your thumb touch.</li>
                    <li><strong>Tension:</strong> Watch for tension in your wrist or forearm. Shake your hand out every minute or so.</li>
                </ul>

                <h3>Exercise 1: Left Hand Mirror Practice</h3>
                <p>Play the C pentascale ascending and descending with your left hand, 10 times. Say the finger numbers out loud as you play: "5, 4, 3, 2, 1, 2, 3, 4, 5."</p>

                <h3>Exercise 2: Left Hand Melody</h3>
                <p>Play "Mary Had a Little Lamb" with your left hand. The note sequence is the same as the right hand version (E, D, C, D, E, E, E...) but the finger numbers are different: 3, 4, 5, 4, 3, 3, 3... Practice slowly until it feels natural.</p>

                <h3>The Left Hand in Real Music</h3>
                <p>In most piano music, the left hand plays one of these roles:</p>
                <ul>
                    <li><strong>Bass notes:</strong> Single low notes that provide the foundation (the "root" of the harmony).</li>
                    <li><strong>Chords:</strong> Groups of notes played together to create harmony under the melody.</li>
                    <li><strong>Accompaniment patterns:</strong> Repeated patterns like the Alberti bass (C-G-E-G) that you will learn later.</li>
                    <li><strong>Counter-melodies:</strong> A second melody line that complements the right hand melody.</li>
                </ul>
                <p>For now, focus on getting comfortable with the basic left hand position. The more natural this feels, the easier the two-hands-together lessons will be.</p>

                <h3>Exercise 3: Left and Right Comparison</h3>
                <p>Play the C pentascale with your right hand (C to G, fingers 1 to 5). Then immediately play the same pentascale with your left hand (C to G, fingers 5 to 1). Compare the feel of each hand. Repeat 5 times, alternating hands each time.</p>

                <h3>Recommended Practice Routine (12 minutes)</h3>
                <ol>
                    <li><strong>2 minutes:</strong> Left hand pentascale, ascending and descending, 10 times.</li>
                    <li><strong>3 minutes:</strong> Isolated finger 5 and finger 4 exercises (press each one firmly, 10 times).</li>
                    <li><strong>3 minutes:</strong> Left hand "Mary Had a Little Lamb" (Exercise 2), slowly.</li>
                    <li><strong>2 minutes:</strong> Left and Right comparison (Exercise 3).</li>
                    <li><strong>2 minutes:</strong> Free play with left hand only. Explore the lower notes of the keyboard.</li>
                </ol>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'In the left hand C position, which finger plays C?',
                    'explanation' => 'In the left hand C position, finger 5 (pinky) plays C. The left hand is a mirror of the right.',
                    'options' => [
                        ['text' => 'Finger 1 (thumb)', 'correct' => false],
                        ['text' => 'Finger 3 (middle)', 'correct' => false],
                        ['text' => 'Finger 5 (pinky)', 'correct' => true],
                        ['text' => 'Finger 2 (index)', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'In the left hand C position, what note does the thumb (finger 1) play?',
                    'explanation' => 'The left hand thumb plays G, the highest note in the C position. The numbering goes 5=C, 4=D, 3=E, 2=F, 1=G.',
                    'options' => [
                        ['text' => 'C', 'correct' => false],
                        ['text' => 'E', 'correct' => false],
                        ['text' => 'F', 'correct' => false],
                        ['text' => 'G', 'correct' => true],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Why does the left hand often feel more difficult than the right hand for most people?',
                    'explanation' => 'Most people are right-handed, so their left hand has less fine motor development and coordination for precise tasks.',
                    'options' => [
                        ['text' => 'The left side of the piano has harder keys', 'correct' => false],
                        ['text' => 'Bass clef notes are more difficult', 'correct' => false],
                        ['text' => 'The non-dominant hand has less developed fine motor skills', 'correct' => true],
                        ['text' => 'Left hand fingers are shorter', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'Your left hand pinky (finger 5) produces a very quiet, weak sound when playing C. What is the best way to strengthen it?',
                    'explanation' => 'Slow, deliberate practice with finger 5, focusing on firm contact and full key depression, builds strength over time.',
                    'options' => [
                        ['text' => 'Avoid using finger 5 and use finger 4 instead', 'correct' => false],
                        ['text' => 'Practice slow, firm presses with finger 5, ensuring full key depression', 'correct' => true],
                        ['text' => 'Press harder with your whole arm', 'correct' => false],
                        ['text' => 'Only practice right hand until left hand gets stronger on its own', 'correct' => false],
                    ]
                ],
            ]
        ],

        [
            'title' => 'Simple Accompaniment',
            'duration' => 12,
            'difficulty' => 3,
            'xp' => 60,
            'video' => '',
            'interactivity' => [
                'type' => 'piano_hero',
                'target' => 3,
                'label' => 'Practice hands together in Piano Hero for 3 minutes'
            ],
            'content' => '
                <h2>Adding a Left Hand Accompaniment</h2>
                <p>Now comes the exciting step: playing <strong>both hands at the same time</strong>. You will start with the simplest possible approach, holding long notes in the left hand while the right hand plays the melody. This is the foundation of all piano accompaniment.</p>

                <h3>The Whole-Note Accompaniment</h3>
                <p>The easiest way to accompany a melody is to play <strong>whole notes</strong> in the left hand while the right hand plays the tune. A whole note lasts 4 beats, so the left hand presses one key and holds it while the right hand plays 4 quarter notes (or equivalent).</p>
                <p>Try this with a simple melody you know. Place your right hand in C position and your left hand pinky (finger 5) on the C below Middle C. Now play:</p>
                <ul>
                    <li><strong>Left hand:</strong> Hold C for 4 beats (whole note)</li>
                    <li><strong>Right hand (at the same time):</strong> Play E, D, C, D (quarter notes, "Mary Had a Little Lamb" opening)</li>
                </ul>

                <h3>How to Combine Hands</h3>
                <p>The key to playing hands together is to <strong>think about the beats, not the hands</strong>. Both hands share the same pulse. Here is the step-by-step approach:</p>
                <ol>
                    <li>Learn the right hand part alone until it is comfortable.</li>
                    <li>Learn the left hand part alone until it is comfortable.</li>
                    <li>Play both together at a <strong>very slow tempo</strong>, slower than you think you need.</li>
                    <li>Focus on what happens on each beat: "On beat 1, both hands play. On beat 2, only the right hand moves."</li>
                </ol>

                <h3>Balance Between Hands</h3>
                <p>When playing melody and accompaniment, the melody (usually right hand) should be <strong>louder</strong> than the accompaniment (left hand). This is called <strong>balance</strong>. Play the right hand at a medium volume (<strong>mezzo forte</strong>) and the left hand softly (<strong>piano</strong>). This ensures listeners hear the tune clearly.</p>

                <h3>Exercise 1: Mary Had a Little Lamb with Accompaniment</h3>
                <p>Right hand plays the melody in C position. Left hand holds a whole note C (below Middle C) throughout. Play the entire song. Change the left hand note to G when the melody reaches the G section, then back to C.</p>

                <h3>Exercise 2: Half Note Accompaniment</h3>
                <p>Upgrade your left hand to half notes: instead of holding one note for 4 beats, play C for 2 beats then G for 2 beats in each measure. This adds gentle motion to the accompaniment while the right hand plays the melody.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What is the simplest type of left hand accompaniment?',
                    'explanation' => 'Whole notes in the left hand are the simplest accompaniment, one note held for an entire measure while the right hand plays the melody.',
                    'options' => [
                        ['text' => 'Eighth note arpeggios', 'correct' => false],
                        ['text' => 'Whole notes held for full measures', 'correct' => true],
                        ['text' => 'Chords changing every beat', 'correct' => false],
                        ['text' => 'Alberti bass patterns', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What does "balance" mean when playing with both hands?',
                    'explanation' => 'Balance means playing the melody louder than the accompaniment so the tune can be clearly heard.',
                    'options' => [
                        ['text' => 'Both hands play at exactly the same volume', 'correct' => false],
                        ['text' => 'The melody is louder than the accompaniment', 'correct' => true],
                        ['text' => 'The left hand plays louder than the right', 'correct' => false],
                        ['text' => 'Both hands play the same notes', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What is the recommended first step for learning a piece with both hands?',
                    'explanation' => 'Learning each hand separately first ensures each part is secure before the challenge of combining them.',
                    'options' => [
                        ['text' => 'Jump straight into playing both hands together', 'correct' => false],
                        ['text' => 'Learn each hand separately, then combine at a slow tempo', 'correct' => true],
                        ['text' => 'Only practice the right hand, the left will follow', 'correct' => false],
                        ['text' => 'Play along with a recording at full speed', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'When combining hands, your left hand keeps "following" the rhythm of the right hand instead of holding its own rhythm. What should you do?',
                    'explanation' => 'Counting beats aloud anchors both hands to the same pulse and prevents one hand from mimicking the other.',
                    'options' => [
                        ['text' => 'Play the right hand louder', 'correct' => false],
                        ['text' => 'Count the beats out loud so both hands follow the same pulse', 'correct' => true],
                        ['text' => 'Stop practicing with both hands and only play right hand', 'correct' => false],
                        ['text' => 'Speed up the tempo', 'correct' => false],
                    ]
                ],
            ]
        ],

        [
            'title' => 'Hand Coordination Drills',
            'duration' => 15,
            'difficulty' => 3,
            'xp' => 70,
            'video' => '',
            'interactivity' => null,
            'content' => '
                <h2>Training Your Hands to Work Together</h2>
                <p>Playing piano with both hands requires your brain to send independent instructions to each hand simultaneously. This is a skill that develops through specific <strong>coordination exercises</strong>. In this lesson, you will practice three types of motion: <strong>contrary motion</strong>, <strong>parallel motion</strong>, and <strong>independent motion</strong>.</p>

                <h3>Contrary Motion</h3>
                <p><strong>Contrary motion</strong> is when both hands move in opposite directions, one goes up while the other goes down. This is often the easiest type of two-hand coordination for beginners because the finger numbers mirror each other.</p>
                <p>Try this: Place both thumbs on Middle C. Play the C pentascale outward, right hand goes up (C, D, E, F, G) while left hand goes down (C, B, A, G, F). Both hands use the same finger numbers: 1, 2, 3, 4, 5.</p>

                <h3>Parallel Motion</h3>
                <p><strong>Parallel motion</strong> is when both hands move in the same direction. This is harder because the finger numbers are different in each hand. When the right hand uses fingers 1-2-3-4-5 going up, the left hand uses 5-4-3-2-1 going up.</p>
                <p>Try this: Place both hands in C position (RH on Middle C area, LH one octave below). Play C-D-E-F-G going up with both hands at the same time. The right hand uses 1-2-3-4-5 and the left hand uses 5-4-3-2-1.</p>

                <h3>Independent Motion</h3>
                <p>The ultimate goal is full <strong>independence</strong>, each hand doing something completely different. Start with this simple exercise: the right hand plays quarter notes (C, D, E, F, G) while the left hand holds a whole note C. Then swap: left hand plays quarter notes while right hand holds.</p>

                <h3>Exercise 1: Contrary Motion Scale</h3>
                <p>Both thumbs on Middle C. Play outward (5 notes) and back inward, 5 times. Keep the tempo perfectly even. Use a metronome at 60 BPM.</p>

                <h3>Exercise 2: One Hand Moves, One Holds</h3>
                <p>Left hand holds C (whole note). Right hand plays: C, D, E, D, C (quarter notes). Then swap: Right hand holds G. Left hand plays: G, F, E, F, G. Repeat 5 times each.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'In contrary motion, the two hands move:',
                    'explanation' => 'Contrary motion means the hands move in opposite directions, one goes up while the other goes down.',
                    'options' => [
                        ['text' => 'In the same direction', 'correct' => false],
                        ['text' => 'In opposite directions', 'correct' => true],
                        ['text' => 'At different speeds', 'correct' => false],
                        ['text' => 'On different beats', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'Why is contrary motion often easier than parallel motion for beginners?',
                    'explanation' => 'In contrary motion, both hands use the same finger numbers simultaneously, which is easier for the brain to coordinate.',
                    'options' => [
                        ['text' => 'The notes sound better together', 'correct' => false],
                        ['text' => 'Both hands use the same finger numbers at the same time', 'correct' => true],
                        ['text' => 'You only need one hand', 'correct' => false],
                        ['text' => 'The tempo is slower', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'In parallel motion going up, if the right hand uses finger 1-2-3-4-5, the left hand uses:',
                    'explanation' => 'Going up in parallel motion, the left hand uses 5-4-3-2-1 because the lowest note uses the pinky and the highest uses the thumb.',
                    'options' => [
                        ['text' => '1-2-3-4-5 (same as right hand)', 'correct' => false],
                        ['text' => '5-4-3-2-1', 'correct' => true],
                        ['text' => '5-3-1-3-5', 'correct' => false],
                        ['text' => '1-3-5-3-1', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You can play each hand separately at 80 BPM, but when combining them you cannot keep up. What tempo should you start at?',
                    'explanation' => 'When combining hands, start at roughly half the tempo you can play hands separately, then gradually increase.',
                    'options' => [
                        ['text' => '80 BPM, the same tempo as hands separate', 'correct' => false],
                        ['text' => '100 BPM, faster to push through the difficulty', 'correct' => false],
                        ['text' => 'Around 40-50 BPM, much slower than hands separate', 'correct' => true],
                        ['text' => 'No metronome, just play freely', 'correct' => false],
                    ]
                ],
            ]
        ],

        [
            'title' => 'Playing Ode to Joy (Both Hands)',
            'duration' => 15,
            'difficulty' => 4,
            'xp' => 70,
            'video' => '',
            'interactivity' => [
                'type' => 'piano_hero',
                'target' => 5,
                'label' => 'Play Ode to Joy with both hands in Piano Hero'
            ],
            'content' => '
                <h2>Your First Two-Hand Song</h2>
                <p>You already know the melody of "Ode to Joy" from Module 1. Now you will play it with <strong>both hands</strong>, melody in the right hand and simple accompaniment in the left. This is a genuine piano arrangement, and completing it is a real achievement.</p>

                <h3>Right Hand: The Melody</h3>
                <p>Place your right hand in C position. Here is the melody you already know:</p>
                <p><strong>E E F G | G F E D | C C D E | E (hold) D (hold)</strong></p>
                <p><strong>E E F G | G F E D | C C D E | D (hold) C (hold)</strong></p>

                <h3>Left Hand: The Accompaniment</h3>
                <p>Your left hand will play simple whole notes and half notes. Place your left hand with finger 5 on the C below Middle C:</p>
                <p><strong>Line 1:</strong> C (whole note) | G (whole note) | C (whole note) | G (whole note)</p>
                <p><strong>Line 2:</strong> C (whole note) | G (whole note) | C (half) G (half) | C (whole note)</p>

                <h3>Step-by-Step Assembly</h3>
                <ol>
                    <li><strong>Step 1:</strong> Play the right hand melody alone, 3 times through.</li>
                    <li><strong>Step 2:</strong> Play the left hand accompaniment alone, 3 times through.</li>
                    <li><strong>Step 3:</strong> Combine just the first 2 measures. Both hands start together on beat 1. Count "1, 2, 3, 4" for each measure.</li>
                    <li><strong>Step 4:</strong> Add measures 3 and 4. Connect to the first 2 measures.</li>
                    <li><strong>Step 5:</strong> Continue adding 2 measures at a time until you can play the whole piece.</li>
                </ol>

                <h3>Exercise 1: First Line, Both Hands</h3>
                <p>At a metronome setting of 50 BPM, play just the first line (4 measures) with both hands. Repeat 5 times, or until you can play it without mistakes.</p>

                <h3>Exercise 2: Complete Performance</h3>
                <p>Play the full "Ode to Joy" arrangement from beginning to end. Use Piano Hero for guided practice with visual feedback. Do not worry about perfection, focus on maintaining a steady tempo and keeping both hands synchronized.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What type of accompaniment does the left hand play in this arrangement of "Ode to Joy"?',
                    'explanation' => 'The left hand plays whole notes (held for 4 beats each), the simplest possible accompaniment pattern.',
                    'options' => [
                        ['text' => 'Chords changing every beat', 'correct' => false],
                        ['text' => 'The same melody as the right hand', 'correct' => false],
                        ['text' => 'Whole notes held for full measures', 'correct' => true],
                        ['text' => 'Eighth note arpeggios', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'When assembling a two-hand piece, how many measures should you add at a time?',
                    'explanation' => 'Adding 2 measures at a time is manageable and allows you to build the piece gradually without overwhelm.',
                    'options' => [
                        ['text' => 'The whole piece at once', 'correct' => false],
                        ['text' => '1-2 measures at a time', 'correct' => true],
                        ['text' => '8 measures at a time', 'correct' => false],
                        ['text' => 'One note at a time', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The left hand note changes from C to G at certain points. These two notes are the roots of which two chords?',
                    'explanation' => 'C is the root of the C major chord (I) and G is the root of the G major chord (V), the two main chords in C major.',
                    'options' => [
                        ['text' => 'D major and A major', 'correct' => false],
                        ['text' => 'C major and G major', 'correct' => true],
                        ['text' => 'F major and C major', 'correct' => false],
                        ['text' => 'E minor and B minor', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You can play measures 1-4 with both hands, but when you try to add measures 5-6, you lose the rhythm. What is the best approach?',
                    'explanation' => 'Practicing the transition zone (measures 4-5) in isolation targets the exact point of difficulty.',
                    'options' => [
                        ['text' => 'Start over from the beginning each time', 'correct' => false],
                        ['text' => 'Practice just measures 4-5 together, isolating the transition point', 'correct' => true],
                        ['text' => 'Skip to measure 5 and practice from there', 'correct' => false],
                        ['text' => 'Play the whole piece faster', 'correct' => false],
                    ]
                ],
            ]
        ],

        [
            'title' => 'Broken Chord Accompaniment',
            'duration' => 15,
            'difficulty' => 4,
            'xp' => 80,
            'video' => '',
            'interactivity' => null,
            'content' => '
                <h2>Beyond Whole Notes: Broken Chords</h2>
                <p>Holding whole notes in the left hand works, but it can sound static. In this lesson, you will learn <strong>broken chord patterns</strong>, ways of playing chord notes one at a time to create a more flowing, musical accompaniment. The most famous of these patterns is the <strong>Alberti bass</strong>.</p>

                <h3>What Is a Broken Chord?</h3>
                <p>A <strong>broken chord</strong> (also called an <strong>arpeggio pattern</strong>) takes the notes of a chord and plays them one at a time instead of all together. For example, instead of pressing C, E, and G simultaneously, you play C, then E, then G in sequence.</p>

                <h3>The Alberti Bass</h3>
                <p>The <strong>Alberti bass</strong> is a specific broken chord pattern that follows this order: <strong>bottom - top - middle - top</strong>. Using the C major chord (C, E, G):</p>
                <p><strong>C - G - E - G - C - G - E - G</strong> (fingers: 5 - 1 - 3 - 1 - 5 - 1 - 3 - 1)</p>
                <p>This pattern was widely used by Classical-era composers like Mozart and Beethoven. It creates a gentle, rocking motion in the accompaniment that supports the melody beautifully.</p>

                <h3>Waltz Bass Pattern</h3>
                <p>Another common left hand pattern is the <strong>waltz bass</strong>, which works perfectly in 3/4 time: play the root note alone on beat 1, then the remaining chord notes together on beats 2 and 3:</p>
                <p><strong>Beat 1: C (alone). Beat 2: E+G (together). Beat 3: E+G (together)</strong></p>
                <p>This "boom-chick-chick" pattern is the signature sound of waltzes and many folk songs.</p>

                <h3>Exercise 1: Alberti Bass Practice</h3>
                <p>Play the Alberti bass pattern on C major (C-G-E-G) with your left hand, in steady eighth notes. Start at 50 BPM. Once comfortable, try the same pattern on F major (F-C-A-C) and G major (G-D-B-D). Then practice switching between them: 4 beats on C, 4 on F, 4 on G, 4 on C.</p>

                <h3>Exercise 2: Melody with Alberti Bass</h3>
                <p>Play "Ode to Joy" melody in your right hand while your left hand plays the Alberti bass pattern. Start extremely slowly, this is significantly harder than whole-note accompaniment. The left hand plays a repeating pattern while the right hand plays different notes. This is real hand independence.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What is the Alberti bass pattern?',
                    'explanation' => 'The Alberti bass plays chord notes in the order: bottom, top, middle, top (e.g., C-G-E-G for C major).',
                    'options' => [
                        ['text' => 'Playing all chord notes together', 'correct' => false],
                        ['text' => 'Bottom-top-middle-top', 'correct' => true],
                        ['text' => 'Bottom-middle-top', 'correct' => false],
                        ['text' => 'Top-bottom-top-bottom', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The waltz bass pattern in 3/4 time follows the structure:',
                    'explanation' => 'Waltz bass plays root alone on beat 1, then chord on beats 2 and 3, creating the "boom-chick-chick" pattern.',
                    'options' => [
                        ['text' => 'Chord-chord-chord', 'correct' => false],
                        ['text' => 'Root-chord-chord (boom-chick-chick)', 'correct' => true],
                        ['text' => 'Root-root-chord', 'correct' => false],
                        ['text' => 'Chord-root-chord', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'For the C major Alberti bass (C-G-E-G), which left hand fingers are used?',
                    'explanation' => 'Finger 5 plays C (bottom), finger 1 plays G (top), finger 3 plays E (middle), finger 1 plays G again.',
                    'options' => [
                        ['text' => '5-3-1-3', 'correct' => false],
                        ['text' => '5-1-3-1', 'correct' => true],
                        ['text' => '1-5-3-5', 'correct' => false],
                        ['text' => '5-2-4-2', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You can play the Alberti bass pattern smoothly with the left hand alone, but when you add the right hand melody, the left hand pattern falls apart. What should you try?',
                    'explanation' => 'The Alberti bass should be automatic (almost like muscle memory) before combining with the right hand. Extra left-hand-alone repetition helps.',
                    'options' => [
                        ['text' => 'Abandon the Alberti bass and go back to whole notes', 'correct' => false],
                        ['text' => 'Practice the left hand pattern until it is automatic, then combine at half tempo', 'correct' => true],
                        ['text' => 'Play the right hand melody faster to match', 'correct' => false],
                        ['text' => 'Only practice hands together, never separately', 'correct' => false],
                    ]
                ],
            ]
        ],

        [
            'title' => 'Playing When the Saints',
            'duration' => 15,
            'difficulty' => 4,
            'xp' => 80,
            'video' => '',
            'interactivity' => [
                'type' => 'piano_hero',
                'target' => 5,
                'label' => 'Play When the Saints in Piano Hero for 5 minutes'
            ],
            'content' => '
                <h2>A New Song: When the Saints Go Marching In</h2>
                <p>"When the Saints Go Marching In" is a joyful, energetic traditional song that is perfect for practicing hands together. It uses a wider range of notes than your previous songs and introduces simple chord accompaniment in the left hand.</p>

                <h3>The Melody (Right Hand)</h3>
                <p>Place your right hand in C position. The melody uses the notes C, D, E, and G:</p>
                <p><strong>Line 1:</strong> C  E  F  G (hold) | C  E  F  G (hold)</p>
                <p><strong>Line 2:</strong> C  E  F  G (hold) E (hold) C | E (hold) D (hold)</p>
                <p><strong>Line 3:</strong> E  E  D  C (hold) E (hold) | G  G  F (hold)</p>
                <p><strong>Line 4:</strong> E  F  G (hold) E (hold) C (hold) | D (hold) C (hold)</p>
                <p>The dashes represent held beats. Practice the right hand alone until you are comfortable with the rhythm.</p>

                <h3>The Accompaniment (Left Hand)</h3>
                <p>The left hand plays simple chord roots as whole notes or half notes:</p>
                <ul>
                    <li>When the melody is around C and E, the left hand plays <strong>C</strong> (root of C major).</li>
                    <li>When the melody moves to G, the left hand plays <strong>G</strong> (root of G major).</li>
                    <li>When the melody resolves back to C, the left hand returns to <strong>C</strong>.</li>
                </ul>

                <h3>Exercise 1: Hands Separate First</h3>
                <p>Practice the right hand 5 times through. Then practice the left hand 5 times through. Only combine them when each hand is secure.</p>

                <h3>Exercise 2: Full Performance</h3>
                <p>Play the complete song with both hands at a slow tempo (60 BPM). Once comfortable, try increasing to 72 BPM. This song should feel upbeat and joyful, let that energy come through in your playing.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'Which notes does the melody of "When the Saints" primarily use?',
                    'explanation' => 'The main melody notes are C, D, E, F, and G, all within the C five-finger position.',
                    'options' => [
                        ['text' => 'Only C and G', 'correct' => false],
                        ['text' => 'C, D, E, F, and G', 'correct' => true],
                        ['text' => 'A, B, C, D, and E', 'correct' => false],
                        ['text' => 'All 7 natural notes', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'In this arrangement, the left hand chord changes from C to G when:',
                    'explanation' => 'The left hand follows the harmony, changing to G when the melody centers around G.',
                    'options' => [
                        ['text' => 'Every other measure', 'correct' => false],
                        ['text' => 'When the melody centers around G', 'correct' => true],
                        ['text' => 'At the beginning of every line', 'correct' => false],
                        ['text' => 'It never changes', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What character should "When the Saints" have when performed?',
                    'explanation' => 'This is a joyful, upbeat traditional song often associated with New Orleans jazz, it should sound energetic and bright.',
                    'options' => [
                        ['text' => 'Slow and melancholy', 'correct' => false],
                        ['text' => 'Upbeat, joyful, and energetic', 'correct' => true],
                        ['text' => 'Quiet and mysterious', 'correct' => false],
                        ['text' => 'Angry and aggressive', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You have learned both hands separately but keep losing your place when combining them. What technique helps most?',
                    'explanation' => 'Counting beats aloud forces both hands to stay synchronized to a shared pulse, preventing drift.',
                    'options' => [
                        ['text' => 'Watch your hands instead of the music', 'correct' => false],
                        ['text' => 'Count beats out loud while playing to keep both hands synchronized', 'correct' => true],
                        ['text' => 'Play without counting and rely on feel', 'correct' => false],
                        ['text' => 'Only practice the parts you already know', 'correct' => false],
                    ]
                ],
            ]
        ],

        [
            'title' => 'Module Review and Challenge',
            'duration' => 15,
            'difficulty' => 5,
            'xp' => 90,
            'video' => '',
            'interactivity' => [
                'type' => 'piano_hero',
                'target' => 10,
                'label' => 'Complete 10 minutes of Piano Hero to finish this module'
            ],
            'content' => '
                <h2>Module 4 Review: Two Hands Together</h2>
                <p>You have achieved something remarkable, playing the piano with both hands. This is the skill that separates dabbling from real piano playing. Let us review and consolidate everything you have learned.</p>

                <h3>Skills Checklist</h3>
                <ul>
                    <li>Play the left hand C position pentascale confidently.</li>
                    <li>Play whole-note and half-note accompaniments with the left hand.</li>
                    <li>Execute contrary motion and parallel motion exercises.</li>
                    <li>Play "Ode to Joy" with both hands.</li>
                    <li>Understand and play the Alberti bass pattern (C-G-E-G).</li>
                    <li>Play the waltz bass pattern.</li>
                    <li>Play "When the Saints Go Marching In" with both hands.</li>
                    <li>Maintain balance between melody and accompaniment.</li>
                </ul>

                <h3>Review Exercise 1: Comprehensive Two-Hand Warm-Up</h3>
                <ol>
                    <li>Contrary motion pentascale from Middle C: 2 times.</li>
                    <li>Parallel motion pentascale in C: 2 times.</li>
                    <li>Alberti bass in C, F, and G: 4 beats each.</li>
                    <li>Right hand melody with left hand whole notes: first line of "Ode to Joy."</li>
                </ol>

                <h3>Review Exercise 2: Song Performance</h3>
                <p>Play both songs you have learned in this module, back to back:</p>
                <ol>
                    <li>"Ode to Joy" with both hands (whole note accompaniment).</li>
                    <li>"When the Saints Go Marching In" with both hands.</li>
                </ol>
                <p>Record yourself if possible. Listen back and evaluate: is the melody clear? Is the left hand steady? Is the tempo even?</p>

                <h3>Piano Hero Challenge</h3>
                <p>Complete 10 minutes of guided practice in <strong>Piano Hero</strong> to finish this module. Choose any beginner song and focus on keeping both hands synchronized with the falling notes.</p>

                <h3>What Comes Next</h3>
                <p>In <strong>Module 5: First Repertoire</strong>, you will add <strong>expression</strong> to your playing, dynamics (loud and soft), articulation (staccato and legato), and you will learn your first classical and pop pieces. You are about to make the leap from playing notes to making music.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What are the three types of hand motion you learned in this module?',
                    'explanation' => 'Contrary motion (opposite directions), parallel motion (same direction), and independent motion (different rhythms) are the three fundamental types.',
                    'options' => [
                        ['text' => 'Fast, medium, and slow motion', 'correct' => false],
                        ['text' => 'Contrary, parallel, and independent motion', 'correct' => true],
                        ['text' => 'Upward, downward, and circular motion', 'correct' => false],
                        ['text' => 'Staccato, legato, and portato motion', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'When playing melody and accompaniment, which hand should generally be louder?',
                    'explanation' => 'The melody hand (usually right hand) should be louder so the tune is clearly audible above the accompaniment.',
                    'options' => [
                        ['text' => 'The left hand (accompaniment)', 'correct' => false],
                        ['text' => 'Both hands equally loud', 'correct' => false],
                        ['text' => 'The right hand (melody)', 'correct' => true],
                        ['text' => 'Neither, both should be very soft', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The Alberti bass pattern for G major (G-B-D) would be:',
                    'explanation' => 'The Alberti bass follows bottom-top-middle-top, so G major becomes G-D-B-D.',
                    'options' => [
                        ['text' => 'G-B-D-B', 'correct' => false],
                        ['text' => 'G-D-B-D', 'correct' => true],
                        ['text' => 'D-G-B-G', 'correct' => false],
                        ['text' => 'B-D-G-D', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You are ready to move to Module 5. How can you confirm you have solid two-hand coordination?',
                    'explanation' => 'Playing a complete song with steady tempo and clear melody/accompaniment balance confirms two-hand readiness.',
                    'options' => [
                        ['text' => 'You can play very fast with one hand', 'correct' => false],
                        ['text' => 'You can read both clefs at the same time', 'correct' => false],
                        ['text' => 'You can play a complete song with both hands at a steady tempo with clear balance', 'correct' => true],
                        ['text' => 'You have memorized 10 chord progressions', 'correct' => false],
                    ]
                ],
            ]
        ],

    ]; // end Module 4 lessons
}

function pm_get_seed_lessons_module5() {
    return [

        // ====================================================================
        // LESSON 1: Dynamics - Piano and Forte
        // ====================================================================
        [
            'title' => 'Dynamics: Piano and Forte',
            'duration' => 12,
            'difficulty' => 2,
            'xp' => 60,
            'video' => '',
            'interactivity' => [
                'type' => 'ear_trainer',
                'target' => 5,
                'label' => 'Identify 5 dynamic levels by ear'
            ],
            'content' => '
                <h2>Playing Soft and Loud</h2>
                <p>Until now, you have focused on playing the right notes at the right time. But music is more than just correct notes, it is about <strong>expression</strong>. One of the most powerful expressive tools is <strong>dynamics</strong>: how loudly or softly you play. In this lesson, you will learn the basic dynamic markings and how to control them on the piano.</p>

                <h3>What Are Dynamics?</h3>
                <p><strong>Dynamics</strong> refers to the volume of sound in music. They are indicated in sheet music by Italian abbreviations written below the staff. Here are the most important dynamic markings for beginners:</p>
                <ul>
                    <li><strong>pp</strong> (pianissimo) = very soft</li>
                    <li><strong>p</strong> (piano) = soft</li>
                    <li><strong>mp</strong> (mezzo piano) = moderately soft</li>
                    <li><strong>mf</strong> (mezzo forte) = moderately loud</li>
                    <li><strong>f</strong> (forte) = loud</li>
                    <li><strong>ff</strong> (fortissimo) = very loud</li>
                </ul>
                <p>Yes, the word "piano" means "soft" in Italian. The full name of the instrument is <strong>pianoforte</strong>, meaning "soft-loud", because it was the first keyboard instrument that could play both soft and loud depending on how you pressed the keys.</p>

                <h3>How to Control Dynamics on the Piano</h3>
                <p>On the piano, volume is controlled by the <strong>speed of your key press</strong>. A faster, more energetic press produces a louder sound. A slower, gentler press produces a softer sound. The key points:</p>
                <ul>
                    <li>For <strong>forte</strong>: use more arm weight and a firm (not tense) finger press. Think of your arm weight dropping into the key.</li>
                    <li>For <strong>piano</strong>: use less arm weight and a gentle finger press. The key still goes all the way down, but more slowly.</li>
                    <li>Avoid the temptation to "hit" for loud or "barely touch" for soft. Both loud and soft should produce a clear, beautiful tone.</li>
                </ul>

                <h3>Gradual Dynamic Changes</h3>
                <p>Music also uses gradual changes in volume:</p>
                <ul>
                    <li><strong>Crescendo</strong> (cresc. or a widening hairpin symbol): gradually getting louder.</li>
                    <li><strong>Decrescendo / Diminuendo</strong> (decresc. or a narrowing hairpin symbol): gradually getting softer.</li>
                </ul>

                <h3>Exercise 1: Dynamic Scale</h3>
                <p>Play the C pentascale (C-D-E-F-G) ascending from <strong>pp</strong> to <strong>ff</strong>, start very soft and get louder with each note. Then play it descending from <strong>ff</strong> to <strong>pp</strong>. This is a crescendo going up and a decrescendo coming down.</p>

                <h3>Exercise 2: "Ode to Joy" with Dynamics</h3>
                <p>Play "Ode to Joy" with these dynamic markings: start the first line at <strong>mf</strong>. Play the second line at <strong>p</strong> (softer). Return to <strong>f</strong> for the final line. Feel how the dynamics create shape and emotion in the music.</p>

                <h3>Why Dynamics Transform Your Playing</h3>
                <p>Listen to any recording of a great pianist and notice how they constantly vary the volume. A piece played entirely at one volume level sounds robotic and lifeless. But add dynamics, even simple ones, and the same notes suddenly sound musical and expressive.</p>
                <p>Here is an analogy: imagine someone speaking in a completely flat, monotone voice versus someone who raises and lowers their voice naturally. The words might be the same, but the delivery makes all the difference. Dynamics are the voice inflection of music.</p>

                <h3>Common Dynamic Mistakes</h3>
                <ul>
                    <li><strong>"Soft" does not mean "weak":</strong> Even at pp, every note should be clear and intentional. Control the speed of your key press, but still make full contact.</li>
                    <li><strong>"Loud" does not mean "harsh":</strong> At ff, avoid pounding. Use arm weight from the shoulder, not muscle tension in the fingers. A warm, full forte sounds powerful without being aggressive.</li>
                    <li><strong>Sudden jumps:</strong> Beginners often jump from soft to loud abruptly. Practice smooth transitions (crescendo/decrescendo) to develop dynamic control.</li>
                </ul>

                <h3>Exercise 3: The Whisper Test</h3>
                <p>Play a single note (Middle C) as softly as you possibly can while still producing a sound. This is your pp. Now play it as loudly as you can without it sounding harsh. This is your ff. Count how many distinct volume levels you can produce between the two extremes. Aim for at least 5 clearly different levels.</p>

                <h3>Recommended Practice Routine (12 minutes)</h3>
                <ol>
                    <li><strong>2 minutes:</strong> The Whisper Test on Middle C (Exercise 3). Find your full dynamic range.</li>
                    <li><strong>3 minutes:</strong> Dynamic Scale (Exercise 1). Crescendo up, decrescendo down.</li>
                    <li><strong>4 minutes:</strong> "Ode to Joy" with dynamics (Exercise 2). Three different volume levels.</li>
                    <li><strong>3 minutes:</strong> Free play: pick any melody you know and experiment with playing it soft, then loud, then with a crescendo. Listen to how dynamics change the emotional impact.</li>
                </ol>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What does the dynamic marking "f" (forte) mean?',
                    'explanation' => 'Forte (f) means loud. It comes from the Italian word for strong.',
                    'options' => [
                        ['text' => 'Very soft', 'correct' => false],
                        ['text' => 'Moderately soft', 'correct' => false],
                        ['text' => 'Loud', 'correct' => true],
                        ['text' => 'Fast', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'How do you play louder on the piano?',
                    'explanation' => 'Louder sound is produced by pressing the key faster with more arm weight, not by hitting or tensing up.',
                    'options' => [
                        ['text' => 'Press the key harder by tensing your fingers', 'correct' => false],
                        ['text' => 'Press the key faster with more arm weight', 'correct' => true],
                        ['text' => 'Hit the key from very high above', 'correct' => false],
                        ['text' => 'Use the sustain pedal', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'A crescendo means:',
                    'explanation' => 'Crescendo means gradually getting louder over a series of notes or measures.',
                    'options' => [
                        ['text' => 'Suddenly very loud', 'correct' => false],
                        ['text' => 'Gradually getting softer', 'correct' => false],
                        ['text' => 'Gradually getting louder', 'correct' => true],
                        ['text' => 'Staying at the same volume', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You try to play piano (softly) but the sound is muffled and unclear. What are you likely doing wrong?',
                    'explanation' => 'Playing softly still requires pressing the key fully to the bottom, just more slowly. Incomplete key depression creates muffled sound.',
                    'options' => [
                        ['text' => 'You are pressing the key too slowly', 'correct' => false],
                        ['text' => 'You are not pressing the key all the way down to the keybed', 'correct' => true],
                        ['text' => 'Your fingers are too curved', 'correct' => false],
                        ['text' => 'You are sitting too far from the piano', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 2: Staccato and Legato
        // ====================================================================
        [
            'title' => 'Staccato and Legato',
            'duration' => 12,
            'difficulty' => 3,
            'xp' => 60,
            'video' => '',
            'interactivity' => null,
            'content' => '
                <h2>Two Ways to Touch: Short and Smooth</h2>
                <p>Beyond volume, another key expressive tool is <strong>articulation</strong>, how you begin and end each note. The two most fundamental articulations are <strong>legato</strong> (smooth and connected) and <strong>staccato</strong> (short and detached). In this lesson, you will learn to play both and feel the difference they make.</p>

                <h3>Legato: Smooth and Connected</h3>
                <p><strong>Legato</strong> (Italian for "tied") means playing notes smoothly, with no gap between them. Each note connects seamlessly to the next. To achieve legato:</p>
                <ul>
                    <li>Hold each key down until the very moment you press the next key.</li>
                    <li>There should be a tiny overlap, the new note begins just as the old note ends.</li>
                    <li>Think of pouring water from one glass to another with no spill. The sound should flow without breaks.</li>
                </ul>
                <p>In sheet music, legato is often indicated by a curved line above or below a group of notes, called a <strong>slur</strong>. A slur tells you to play all notes under it in one smooth, connected phrase.</p>

                <h3>Staccato: Short and Detached</h3>
                <p><strong>Staccato</strong> (Italian for "detached") means playing notes short and separated, with a small silence between each note. To achieve staccato:</p>
                <ul>
                    <li>Press the key and release it quickly, the note lasts about half its written value.</li>
                    <li>Use a light, bouncy wrist motion, like touching a hot surface briefly.</li>
                    <li>The sound should be crisp and clean, not aggressive.</li>
                </ul>
                <p>In sheet music, staccato is indicated by a small <strong>dot</strong> placed directly above or below the note head.</p>

                <h3>The Musical Impact</h3>
                <p>Legato and staccato completely change the character of a passage:</p>
                <ul>
                    <li><strong>Legato</strong> creates a singing, lyrical quality, perfect for melodies and slow passages.</li>
                    <li><strong>Staccato</strong> creates a playful, energetic quality, perfect for dance-like passages and rhythmic patterns.</li>
                </ul>

                <h3>Exercise 1: Side by Side Comparison</h3>
                <p>Play the C pentascale (C-D-E-F-G) two ways:</p>
                <ol>
                    <li>First, play it completely <strong>legato</strong>, hold each note until the next begins. Listen for the smooth, connected sound.</li>
                    <li>Then play it completely <strong>staccato</strong>, make each note short and bouncy. Listen for the crisp, detached sound.</li>
                </ol>
                <p>The notes are the same, but the musical effect is entirely different.</p>

                <h3>Exercise 2: Articulation in a Song</h3>
                <p>Play "Mary Had a Little Lamb" first entirely legato, then entirely staccato. Which version sounds more like you would want to hear it? Most melodies sound best legato, but experimenting with both helps you develop control over your touch.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What does "legato" mean?',
                    'explanation' => 'Legato means smooth and connected, with no gaps between notes.',
                    'options' => [
                        ['text' => 'Short and bouncy', 'correct' => false],
                        ['text' => 'Very loud', 'correct' => false],
                        ['text' => 'Smooth and connected', 'correct' => true],
                        ['text' => 'Very fast', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'How is staccato indicated in sheet music?',
                    'explanation' => 'A small dot placed directly above or below the note head indicates staccato.',
                    'options' => [
                        ['text' => 'A curved line over the notes', 'correct' => false],
                        ['text' => 'A small dot above or below the note', 'correct' => true],
                        ['text' => 'The letter "S" above the note', 'correct' => false],
                        ['text' => 'A wavy line under the note', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'To play staccato correctly, you should:',
                    'explanation' => 'Staccato requires pressing and quickly releasing the key with a light, bouncy wrist motion.',
                    'options' => [
                        ['text' => 'Press the key very hard and hold it long', 'correct' => false],
                        ['text' => 'Not press the key all the way down', 'correct' => false],
                        ['text' => 'Press and quickly release with a light, bouncy motion', 'correct' => true],
                        ['text' => 'Use the sustain pedal', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You see a curved line (slur) above a group of 4 notes in your sheet music. How should you play them?',
                    'explanation' => 'A slur indicates legato, play all notes under the slur smoothly connected, with no gaps between them.',
                    'options' => [
                        ['text' => 'Play them staccato (short and detached)', 'correct' => false],
                        ['text' => 'Play them all at the same time as a chord', 'correct' => false],
                        ['text' => 'Play them legato (smooth and connected)', 'correct' => true],
                        ['text' => 'Play them very loudly', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 3: Playing Für Elise (Simplified)
        // ====================================================================
        [
            'title' => 'Playing Fur Elise (Simplified)',
            'duration' => 15,
            'difficulty' => 4,
            'xp' => 80,
            'video' => '',
            'interactivity' => [
                'type' => 'piano_hero',
                'target' => 5,
                'label' => 'Practice Fur Elise theme in Piano Hero for 5 minutes'
            ],
            'content' => '
                <h2>Your First Classical Piece</h2>
                <p>Beethoven\'s "Fur Elise" is one of the most recognizable piano melodies in the world. The full piece is at an intermediate level, but the famous opening theme is accessible to beginners. In this lesson, you will learn a simplified version of this iconic theme.</p>

                <h3>About the Piece</h3>
                <p>"Fur Elise" (meaning "For Elise") was composed by Ludwig van Beethoven around 1810. The piece is in <strong>A minor</strong>, which means it uses mostly white keys but has a slightly sad, mysterious character compared to the bright sound of C major. The opening theme uses a back-and-forth pattern that is instantly recognizable.</p>

                <h3>The Simplified Theme (Right Hand)</h3>
                <p>The opening motif uses these notes, played in a flowing pattern:</p>
                <p><strong>E  D#  E  D#  E  B  D  C  |  A (hold)</strong></p>
                <p>Wait, D# (D sharp)? This is your first encounter with a <strong>black key</strong> in a melody. D-sharp is the black key immediately to the right of D. Press it with finger 2, the same finger you would use for D, but shifted slightly to the right.</p>
                <p>The pattern continues:</p>
                <p><strong>C  E  A  |  B (hold) |  E  G#  B  |  C (hold)</strong></p>
                <p>G-sharp is another black key, the one between G and A.</p>

                <h3>Why This Piece Sounds Special</h3>
                <p>The alternating E and D-sharp at the beginning creates a <strong>trill-like</strong> effect, a gentle oscillation between two notes that creates tension. When it finally resolves down to A, you feel a sense of arrival. This tension-and-release is at the heart of musical expression.</p>

                <h3>Left Hand (Simplified)</h3>
                <p>For the simplified version, the left hand plays single bass notes:</p>
                <ul>
                    <li>A (when the right hand plays the E-D#-E pattern and resolves to A)</li>
                    <li>E (when the right hand plays the second phrase)</li>
                    <li>A (for the return of the main theme)</li>
                </ul>

                <h3>Exercise 1: Learn the Right Hand Theme</h3>
                <p>Practice just the first 2 measures of the right hand theme, very slowly. Focus on the smooth transition to and from the black key D-sharp. Play it 10 times until it flows naturally.</p>

                <h3>Exercise 2: Add Left Hand Bass Notes</h3>
                <p>Once the right hand is comfortable, add the left hand bass note (A) on beat 1 of the measures where the melody resolves. Start at a very slow tempo (40 BPM) and gradually increase.</p>

                <h3>Practice Tips</h3>
                <p>You can explore the full score of "Fur Elise" on PianoMode\'s Listen and Play page. Listen to the interactive playback to hear how the piece should sound, and compare it to your simplified version. As you improve, you can learn more sections of this beautiful piece.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'Who composed "Fur Elise"?',
                    'explanation' => 'Ludwig van Beethoven composed "Fur Elise" around 1810.',
                    'options' => [
                        ['text' => 'Mozart', 'correct' => false],
                        ['text' => 'Bach', 'correct' => false],
                        ['text' => 'Beethoven', 'correct' => true],
                        ['text' => 'Chopin', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'D-sharp is located:',
                    'explanation' => 'D-sharp is the black key immediately to the right of D on the keyboard.',
                    'options' => [
                        ['text' => 'The white key between D and E', 'correct' => false],
                        ['text' => 'The black key immediately to the right of D', 'correct' => true],
                        ['text' => 'The black key to the left of D', 'correct' => false],
                        ['text' => 'Two keys to the right of D', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => '"Fur Elise" is in the key of:',
                    'explanation' => '"Fur Elise" is in A minor, which gives it a slightly sad, mysterious character.',
                    'options' => [
                        ['text' => 'C major', 'correct' => false],
                        ['text' => 'G major', 'correct' => false],
                        ['text' => 'A minor', 'correct' => true],
                        ['text' => 'D major', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'The alternating E and D-sharp in the opening of "Fur Elise" creates what musical effect?',
                    'explanation' => 'The back-and-forth between two adjacent notes creates tension, which resolves when the melody moves to a new note (A).',
                    'options' => [
                        ['text' => 'A chord progression', 'correct' => false],
                        ['text' => 'Tension through oscillation, which resolves when the melody descends', 'correct' => true],
                        ['text' => 'A scale pattern', 'correct' => false],
                        ['text' => 'A rhythmic acceleration', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 4: Playing a Pop Song (Simplified)
        // ====================================================================
        [
            'title' => 'Playing a Pop Song',
            'duration' => 15,
            'difficulty' => 4,
            'xp' => 80,
            'video' => '',
            'interactivity' => [
                'type' => 'piano_hero',
                'target' => 5,
                'label' => 'Practice the pop song arrangement in Piano Hero'
            ],
            'content' => '
                <h2>Your First Pop Arrangement</h2>
                <p>Piano is not just for classical music. Some of the most beloved songs in popular music are built on simple piano parts. In this lesson, you will learn how pop piano arrangements work and play a simplified version of a popular chord-based song.</p>

                <h3>How Pop Piano Works</h3>
                <p>Most pop songs are built on <strong>chord progressions</strong>, repeating sequences of chords that loop throughout the song. The piano player\'s job is to play these chords in various patterns while a singer or another instrument carries the melody.</p>
                <p>Many hit songs use just 3 or 4 chords. You already know C major, F major, and G major from Module 2. With the addition of <strong>A minor</strong> (A-C-E), you have the four chords behind thousands of popular songs.</p>

                <h3>The A Minor Chord</h3>
                <p><strong>A minor</strong> is built from the notes <strong>A, C, and E</strong>. Notice that it uses the same notes as C major (C-E-G) but with A as the starting point and without the G. The minor chord has a sadder, more introspective sound compared to a major chord.</p>
                <p>Practice playing A minor: press A, C, and E simultaneously with your right hand. Compare the sound to C major (C-E-G). The difference in mood is striking.</p>

                <h3>The Four-Chord Progression</h3>
                <p>One of the most common chord progressions in pop music is: <strong>C → G → Am → F</strong> (or in Roman numerals: I - V - vi - IV). This progression appears in songs across decades and genres.</p>
                <p>Play each chord as a block (all notes together) and hold for 4 beats:</p>
                <p><strong>C major (4 beats) → G major (4 beats) → A minor (4 beats) → F major (4 beats)</strong></p>
                <p>Repeat this loop several times. You are now playing the harmonic foundation of countless hit songs.</p>

                <h3>Adding a Rhythm Pattern</h3>
                <p>Instead of holding each chord for 4 beats, try playing it in a rhythmic pattern:</p>
                <ul>
                    <li><strong>Pattern 1:</strong> Play the chord on beat 1, rest on beats 2-3-4. (Simple.)</li>
                    <li><strong>Pattern 2:</strong> Play on beats 1 and 3. (More movement.)</li>
                    <li><strong>Pattern 3:</strong> Play on "1, 2-and, 4" for a gentle pop feel.</li>
                </ul>

                <h3>Exercise 1: Chord Progression Loop</h3>
                <p>Play the C → G → Am → F progression in a loop, 4 times through. Use Pattern 2 (chords on beats 1 and 3). Try to make the transitions between chords as smooth as possible.</p>

                <h3>Exercise 2: Add a Simple Melody</h3>
                <p>While the left hand plays the chord roots (C, G, A, F as single notes), try humming or singing a melody you know over the top. Then try playing a simple right hand melody over the left hand chords. This is how pop piano works in practice.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What notes make up the A minor chord?',
                    'explanation' => 'A minor is built from A (root), C (minor third), and E (fifth).',
                    'options' => [
                        ['text' => 'A, C#, E', 'correct' => false],
                        ['text' => 'A, C, E', 'correct' => true],
                        ['text' => 'A, D, F', 'correct' => false],
                        ['text' => 'A, B, E', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The common pop progression C → G → Am → F uses how many different chords?',
                    'explanation' => 'This progression uses 4 different chords: C major, G major, A minor, and F major.',
                    'options' => [
                        ['text' => '2', 'correct' => false],
                        ['text' => '3', 'correct' => false],
                        ['text' => '4', 'correct' => true],
                        ['text' => '5', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What is the main difference in sound between a major chord and a minor chord?',
                    'explanation' => 'Major chords sound bright and happy, while minor chords sound sadder and more introspective.',
                    'options' => [
                        ['text' => 'Major is louder, minor is softer', 'correct' => false],
                        ['text' => 'Major sounds bright/happy, minor sounds sad/introspective', 'correct' => true],
                        ['text' => 'Major has 3 notes, minor has 2', 'correct' => false],
                        ['text' => 'There is no audible difference', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You want to learn a pop song on piano but do not have sheet music. What is the most useful thing to know?',
                    'explanation' => 'Knowing the chord progression allows you to play along with most pop songs, since pop music is built on repeating chord patterns.',
                    'options' => [
                        ['text' => 'The exact melody notes', 'correct' => false],
                        ['text' => 'The chord progression', 'correct' => true],
                        ['text' => 'The time signature', 'correct' => false],
                        ['text' => 'The key signature', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 5: Building a Practice Routine
        // ====================================================================
        [
            'title' => 'Building a Practice Routine',
            'duration' => 12,
            'difficulty' => 2,
            'xp' => 60,
            'video' => '',
            'interactivity' => null,
            'content' => '
                <h2>How to Practice Effectively</h2>
                <p>Talent is important, but <strong>consistent, structured practice</strong> is what truly builds piano skills. A student who practices 20 focused minutes daily will progress far faster than someone who practices 2 hours once a week. In this lesson, you will build your personal practice routine.</p>

                <h3>The Ideal Beginner Practice Session (20–30 Minutes)</h3>
                <p>Here is a recommended structure for your daily practice:</p>
                <ol>
                    <li><strong>Warm-Up (5 minutes):</strong> Pentascales in C and G (both hands), contrary motion from Middle C, or Hanon-style finger exercises. This wakes up your fingers and focuses your mind.</li>
                    <li><strong>Technique (5 minutes):</strong> Work on the specific skill you are currently learning, chord transitions, Alberti bass patterns, or a new hand coordination exercise.</li>
                    <li><strong>Repertoire (10 minutes):</strong> Practice the piece(s) you are currently learning. Focus on trouble spots, not just playing through from beginning to end.</li>
                    <li><strong>Sight Reading or Ear Training (5 minutes):</strong> Use PianoMode\'s Sight Reading app or Ear Trainer. Even 5 minutes daily makes a huge difference over time.</li>
                    <li><strong>Fun/Free Play (5 minutes, optional):</strong> Play whatever you enjoy. Revisit a favorite song, improvise, or explore new sounds. This keeps your love for music alive.</li>
                </ol>

                <h3>Practice Principles</h3>
                <ul>
                    <li><strong>Consistency over duration:</strong> 20 minutes daily beats 2 hours on weekends.</li>
                    <li><strong>Slow practice:</strong> If you are making errors, you are playing too fast. Slow down until you can play perfectly, then gradually increase tempo.</li>
                    <li><strong>Isolate problem spots:</strong> Do not just play through the whole piece over and over. Identify the 2–3 bars that give you trouble and loop those bars repeatedly.</li>
                    <li><strong>Use a metronome:</strong> Set it at a comfortable tempo and increase by 2–4 BPM only when you can play perfectly at the current tempo.</li>
                    <li><strong>Take breaks:</strong> If frustration builds, step away for a few minutes. Your brain continues processing even when you are not playing.</li>
                </ul>

                <h3>Exercise 1: Design Your Routine</h3>
                <p>Write out your personal practice schedule. Decide what time of day you will practice, how long each section will be, and what specific exercises you will include. Pin it near your piano as a reminder.</p>

                <h3>Exercise 2: The 5-Day Challenge</h3>
                <p>Commit to practicing your routine for 5 consecutive days. Track your practice using PianoMode\'s streak system. After 5 days, evaluate: what went well? What would you change?</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'What is the most important factor in piano practice?',
                    'explanation' => 'Regular daily practice creates stronger neural pathways than occasional long sessions.',
                    'options' => [
                        ['text' => 'Practicing for as long as possible in each session', 'correct' => false],
                        ['text' => 'Consistent daily practice, even if short', 'correct' => true],
                        ['text' => 'Only practicing on weekends', 'correct' => false],
                        ['text' => 'Playing the same piece repeatedly without stopping', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'When you make repeated errors in a specific passage, you should:',
                    'explanation' => 'Isolating the problem bars and repeating them slowly targets the exact difficulty, which is more effective than playing the whole piece.',
                    'options' => [
                        ['text' => 'Skip that passage and come back to it later', 'correct' => false],
                        ['text' => 'Play the whole piece faster to get past the hard part', 'correct' => false],
                        ['text' => 'Isolate those specific bars and practice them slowly', 'correct' => true],
                        ['text' => 'Move on to a different piece', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'How should you increase tempo when learning a piece?',
                    'explanation' => 'Increase by small increments (2-4 BPM) only after you can play perfectly at the current tempo.',
                    'options' => [
                        ['text' => 'Double the tempo each day', 'correct' => false],
                        ['text' => 'Increase by 2-4 BPM after playing perfectly at current tempo', 'correct' => true],
                        ['text' => 'Start at performance tempo and slow down for mistakes', 'correct' => false],
                        ['text' => 'Never use a metronome', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You have been practicing the same piece for 30 minutes and feel frustrated. What is the best action?',
                    'explanation' => 'Taking a short break relieves frustration and allows your brain to consolidate what it has been processing.',
                    'options' => [
                        ['text' => 'Push through for another 30 minutes', 'correct' => false],
                        ['text' => 'Take a 5-minute break, then return with fresh focus', 'correct' => true],
                        ['text' => 'Give up for the day and try tomorrow', 'correct' => false],
                        ['text' => 'Play the piece faster to finish quickly', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 6: Mini Recital Preparation
        // ====================================================================
        [
            'title' => 'Mini Recital Preparation',
            'duration' => 15,
            'difficulty' => 4,
            'xp' => 80,
            'video' => '',
            'interactivity' => null,
            'content' => '
                <h2>Performing for Others</h2>
                <p>Music is meant to be shared. Playing for an audience, even an audience of one, is a powerful experience that brings all your skills together. In this lesson, you will prepare a mini recital: a short performance of 2–3 pieces for friends, family, or even just yourself on a recording.</p>

                <h3>Choosing Your Program</h3>
                <p>Select 2–3 pieces that you can play confidently. A good mini recital program has <strong>variety</strong>:</p>
                <ul>
                    <li>One piece you know very well (your safest choice to start with confidence).</li>
                    <li>One piece that shows a different style or mood.</li>
                    <li>Optionally, a short encore piece, something fun and light.</li>
                </ul>
                <p>From what you have learned so far, you could choose from: "Ode to Joy" (both hands), "Twinkle, Twinkle, Little Star," the "Fur Elise" theme, "When the Saints," or a pop chord progression.</p>

                <h3>Performance Etiquette</h3>
                <ul>
                    <li><strong>Walk to the piano with confidence.</strong> Even if you feel nervous, carry yourself calmly.</li>
                    <li><strong>Sit down, adjust your bench</strong>, and take a breath before playing.</li>
                    <li><strong>Pause for a moment</strong>, set the tempo in your mind before the first note.</li>
                    <li><strong>If you make a mistake, keep going.</strong> Your audience likely will not notice small errors. What they will notice is if you stop and restart.</li>
                    <li><strong>After finishing, pause</strong> with your hands on the keys for a moment, then stand and bow or acknowledge the audience with a smile.</li>
                </ul>

                <h3>Dealing with Performance Nerves</h3>
                <p>Feeling nervous before performing is completely normal, even world-class pianists experience it. Here are strategies that help:</p>
                <ul>
                    <li><strong>Breathe:</strong> Take 3 slow, deep breaths before sitting down. This calms your heart rate.</li>
                    <li><strong>Focus on the music, not yourself:</strong> Instead of worrying about being watched, concentrate on the sounds you are creating.</li>
                    <li><strong>Practice performing:</strong> Play for one person before the real event. The more you perform, the easier it gets.</li>
                    <li><strong>Accept imperfection:</strong> No performance is flawless. What matters is the musical experience you create.</li>
                </ul>

                <h3>Exercise 1: Dress Rehearsal</h3>
                <p>Set up a "recital" at home. Clear a space, put on shoes (as if performing), and play your 2–3 pieces from beginning to end without stopping. Record yourself with your phone. Watch the recording and note one thing you did well and one thing to improve.</p>

                <h3>Exercise 2: Play for Someone</h3>
                <p>Play your mini recital for at least one other person, a family member, friend, or even a pet. The act of playing for a listener activates different parts of your brain than practicing alone, and it is an essential skill to develop early.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'If you make a mistake during a performance, what should you do?',
                    'explanation' => 'The golden rule of performance: keep going. Stopping or restarting is more noticeable than a small error.',
                    'options' => [
                        ['text' => 'Stop and start the piece over', 'correct' => false],
                        ['text' => 'Apologize to the audience', 'correct' => false],
                        ['text' => 'Keep going without stopping', 'correct' => true],
                        ['text' => 'Play the wrong note again to confirm it', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'A good mini recital program should include:',
                    'explanation' => 'Variety in style or mood keeps the audience engaged and showcases different skills.',
                    'options' => [
                        ['text' => 'The same piece played three times', 'correct' => false],
                        ['text' => 'Only the hardest piece you know', 'correct' => false],
                        ['text' => '2-3 pieces with variety in style or mood', 'correct' => true],
                        ['text' => 'Only pieces you just started learning', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'What should you do before playing the first note in a performance?',
                    'explanation' => 'Taking a moment to breathe and set the tempo mentally ensures a calm, confident start.',
                    'options' => [
                        ['text' => 'Start playing immediately after sitting down', 'correct' => false],
                        ['text' => 'Take a breath, pause, and set the tempo in your mind', 'correct' => true],
                        ['text' => 'Tell the audience which notes you will play', 'correct' => false],
                        ['text' => 'Warm up with scales on stage', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'You feel extremely nervous about playing for your family. What is the most effective strategy?',
                    'explanation' => 'Gradual exposure (playing for one person first) builds performance confidence incrementally.',
                    'options' => [
                        ['text' => 'Avoid performing until you feel completely ready', 'correct' => false],
                        ['text' => 'Start by playing for just one person to build confidence gradually', 'correct' => true],
                        ['text' => 'Play only when no one is home', 'correct' => false],
                        ['text' => 'Take medication before playing', 'correct' => false],
                    ]
                ],
            ]
        ],

        // ====================================================================
        // LESSON 7: Beginner Level Graduation
        // ====================================================================
        [
            'title' => 'Beginner Graduation',
            'duration' => 15,
            'difficulty' => 5,
            'xp' => 100,
            'video' => '',
            'interactivity' => [
                'type' => 'ear_trainer',
                'target' => 15,
                'label' => 'Complete 15 Ear Trainer questions for your graduation exam'
            ],
            'content' => '
                <h2>Congratulations, You Are a Pianist</h2>
                <p>This is the final lesson of the Beginner level. Take a moment to appreciate what you have accomplished. You started with zero knowledge of the piano, and now you can play melodies with both hands, read basic notation, understand rhythm and chords, and perform pieces with expression. That is an extraordinary achievement.</p>

                <h3>Everything You Have Learned</h3>
                <p>Over 5 modules and 30 lessons, you have mastered:</p>
                <ul>
                    <li><strong>Keyboard geography:</strong> The layout of white and black keys, octaves, and Middle C.</li>
                    <li><strong>Posture and technique:</strong> Proper bench height, hand shape, finger numbers, touch technique.</li>
                    <li><strong>Note reading:</strong> The grand staff, treble and bass clefs, ledger lines, and basic sight reading.</li>
                    <li><strong>Rhythm:</strong> Whole, half, quarter, and eighth notes, rests, and time signatures (4/4, 3/4, 2/4).</li>
                    <li><strong>Chords:</strong> C major, F major, G major, A minor, and the I-IV-V progression.</li>
                    <li><strong>Two-hand playing:</strong> Melody with accompaniment, contrary and parallel motion, Alberti bass.</li>
                    <li><strong>Expression:</strong> Dynamics (p, mf, f), articulation (legato, staccato), and performance skills.</li>
                    <li><strong>Repertoire:</strong> "Mary Had a Little Lamb," "Twinkle Twinkle," "Ode to Joy," "Fur Elise" theme, "When the Saints," and pop chord progressions.</li>
                </ul>

                <h3>Graduation Assessment</h3>
                <p>To complete the Beginner level, you must:</p>
                <ol>
                    <li>Pass all module quizzes with 80% or higher.</li>
                    <li>Complete the Ear Trainer graduation challenge (15 questions).</li>
                    <li>Have logged at least 30 minutes of Piano Hero practice across all modules.</li>
                </ol>

                <h3>What Comes Next: Elementary Level</h3>
                <p>The <strong>Elementary level</strong> will take your skills to the next stage. Here is what awaits you:</p>
                <ul>
                    <li><strong>Hand Independence:</strong> True polyphonic playing with different rhythms in each hand.</li>
                    <li><strong>Scales and Arpeggios:</strong> C, G, D, A, and F major scales with proper fingering (thumb-under technique).</li>
                    <li><strong>Theory:</strong> Intervals, inversions, the Circle of Fifths, key signatures, and transposition.</li>
                    <li><strong>Repertoire:</strong> Bach\'s Minuet in G, Clementi Sonatinas, and popular song arrangements.</li>
                    <li><strong>Performance:</strong> Memorization, pedal technique, and stage presence.</li>
                </ul>

                <h3>A Note of Encouragement</h3>
                <p>Learning piano is a lifelong journey. There will be days when practice feels difficult and progress seems slow. That is normal, even the greatest pianists experienced the same struggles. What sets successful musicians apart is their willingness to show up every day and practice with patience and intention.</p>
                <p>You have already proven you can do this. Keep going. The music only gets more beautiful from here.</p>
            ',
            'challenges' => [
                [
                    'type' => 'SELECT',
                    'question' => 'Which of the following is NOT a note value you learned in the Beginner level?',
                    'explanation' => 'Sixteenth notes were not covered in the Beginner level. You learned whole, half, quarter, and eighth notes.',
                    'options' => [
                        ['text' => 'Whole note', 'correct' => false],
                        ['text' => 'Half note', 'correct' => false],
                        ['text' => 'Sixteenth note', 'correct' => true],
                        ['text' => 'Eighth note', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The four chords you learned across the Beginner level are:',
                    'explanation' => 'C major, F major, G major, and A minor are the four chords covered, forming the backbone of thousands of songs.',
                    'options' => [
                        ['text' => 'C major, D major, E major, F major', 'correct' => false],
                        ['text' => 'C major, F major, G major, A minor', 'correct' => true],
                        ['text' => 'A major, B major, C major, D major', 'correct' => false],
                        ['text' => 'C minor, F minor, G minor, A minor', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'SELECT',
                    'question' => 'The Alberti bass pattern plays chord notes in which order?',
                    'explanation' => 'The Alberti bass follows the pattern bottom-top-middle-top (e.g., C-G-E-G for C major).',
                    'options' => [
                        ['text' => 'Bottom-middle-top', 'correct' => false],
                        ['text' => 'Top-middle-bottom', 'correct' => false],
                        ['text' => 'Bottom-top-middle-top', 'correct' => true],
                        ['text' => 'Middle-bottom-top', 'correct' => false],
                    ]
                ],
                [
                    'type' => 'ASSIST',
                    'question' => 'A friend who is thinking about learning piano asks you: "What is the single most important thing for a beginner?" Based on your experience, what would you say?',
                    'explanation' => 'Consistent daily practice, even just 15-20 minutes, is the foundation of all piano progress.',
                    'options' => [
                        ['text' => 'Buy the most expensive piano you can afford', 'correct' => false],
                        ['text' => 'Practice consistently every day, even if just for 15-20 minutes', 'correct' => true],
                        ['text' => 'Learn music theory before touching the piano', 'correct' => false],
                        ['text' => 'Start with the hardest piece you can find', 'correct' => false],
                    ]
                ],
            ]
        ],

    ]; // end Module 5 lessons
}

/**
 * Main seed function - creates levels, modules, lessons, and challenges
 */
function pm_seed_content() {
    // Only run once per version
    $seed_version = '4.0';
    if (get_option('pm_seed_content_version') === $seed_version) return;

    // Only admins
    if (!current_user_can('manage_options')) return;

    global $wpdb;
    $prefix = $wpdb->prefix;

    // ========================================
    // 1. Create All Levels
    // ========================================
    $levels = [
        ['Beginner', 'beginner', 'Start your piano journey from scratch. Learn posture, hand position, basic notes, and your first melodies.'],
        ['Elementary', 'elementary', 'Build on basics with scales, chords, and simple songs.'],
        ['Intermediate', 'intermediate', 'Develop technique, sight-reading, and musical expression.'],
        ['Advanced', 'advanced', 'Master complex pieces, advanced harmony, and performance skills.'],
        ['Expert', 'expert', 'Concert-level repertoire, improvisation, and artistic mastery.'],
    ];

    foreach ($levels as $lv) {
        if (!term_exists($lv[1], 'pm_level')) {
            wp_insert_term($lv[0], 'pm_level', ['slug' => $lv[1], 'description' => $lv[2]]);
        }
    }

    $beginner = term_exists('beginner', 'pm_level');
    $beginner_id = is_array($beginner) ? $beginner['term_id'] : $beginner;

    // ========================================
    // 2. Define All 5 Beginner Modules
    // ========================================
    $modules = [
        [
            'name' => 'Piano Discovery',
            'slug' => 'piano-discovery',
            'description' => 'Your first steps at the piano. Learn the keyboard layout, proper posture, and play your first notes and melodies.',
            'order' => 1,
            'lessons_fn' => 'pm_get_seed_lessons_module1',
        ],
        [
            'name' => 'Expanding Range',
            'slug' => 'expanding-range',
            'description' => 'Extend your note range beyond C position. Learn G position, basic chords, eighth notes, and play your first complete songs.',
            'order' => 2,
            'lessons_fn' => 'pm_get_seed_lessons_module2',
        ],
        [
            'name' => 'Reading Music',
            'slug' => 'reading-music',
            'description' => 'Learn to read standard music notation, the treble clef, bass clef, grand staff, and time signatures. Start reading simple melodies from sheet music.',
            'order' => 3,
            'lessons_fn' => 'pm_get_seed_lessons_module3',
        ],
        [
            'name' => 'Two Hands Together',
            'slug' => 'two-hands-together',
            'description' => 'The moment you have been waiting for, playing with both hands. Learn left hand positions, simple accompaniment patterns, and hand coordination.',
            'order' => 4,
            'lessons_fn' => 'pm_get_seed_lessons_module4',
        ],
        [
            'name' => 'First Repertoire',
            'slug' => 'first-repertoire',
            'description' => 'Make music come alive with dynamics, articulation, and expression. Learn your first classical and pop pieces, build a practice routine, and prepare for your first mini recital.',
            'order' => 5,
            'lessons_fn' => 'pm_get_seed_lessons_module5',
        ],
    ];

    $created_ids = [];

    // ========================================
    // 3. Seed Each Module + Lessons
    // ========================================
    foreach ($modules as $mod) {
        // Create module term
        $term = term_exists($mod['slug'], 'pm_module');
        if (!$term) {
            $term = wp_insert_term($mod['name'], 'pm_module', [
                'slug' => $mod['slug'],
                'description' => $mod['description'],
            ]);
        }
        $module_id = is_array($term) ? $term['term_id'] : $term;

        // Store module order and level association
        update_term_meta($module_id, '_pm_module_order', $mod['order']);
        update_term_meta($module_id, '_pm_module_level', 'beginner');

        // Get lessons from the seed function
        $fn = $mod['lessons_fn'];
        if (!function_exists($fn)) continue;
        $lessons = $fn();

        foreach ($lessons as $i => $lesson) {
            // Check if already exists
            $existing = get_page_by_title($lesson['title'], OBJECT, 'pm_lesson');
            if ($existing) {
                $created_ids[] = $existing->ID;
                continue;
            }

            $post_id = wp_insert_post([
                'post_title'   => $lesson['title'],
                'post_content' => $lesson['content'],
                'post_status'  => 'publish',
                'post_type'    => 'pm_lesson',
                'post_name'    => sanitize_title($lesson['title']),
            ]);

            if (is_wp_error($post_id)) continue;

            // Assign taxonomies
            wp_set_object_terms($post_id, (int) $beginner_id, 'pm_level');
            wp_set_object_terms($post_id, (int) $module_id, 'pm_module');

            // Auto-tag lessons based on title/content keywords
            if (taxonomy_exists('pm_lesson_tag')) {
                $auto_tags = [];
                $title_lower = strtolower($lesson['title']);
                $content_lower = strtolower($lesson['content']);
                $tag_keywords = [
                    'keyboard' => ['keyboard', 'keys', 'piano keys'],
                    'notes' => ['notes', 'note', 'notation'],
                    'rhythm' => ['rhythm', 'tempo', 'beat', 'time signature'],
                    'chords' => ['chord', 'triad', 'harmony'],
                    'scales' => ['scale', 'major scale', 'minor scale'],
                    'melody' => ['melody', 'melodies', 'tune'],
                    'posture' => ['posture', 'hand position', 'finger'],
                    'reading' => ['reading', 'sight-read', 'sheet music', 'staff'],
                    'ear-training' => ['ear train', 'listening', 'identify'],
                    'technique' => ['technique', 'finger exercise', 'dexterity'],
                    'dynamics' => ['dynamics', 'forte', 'piano', 'volume'],
                    'practice' => ['practice', 'exercise', 'drill'],
                ];
                foreach ($tag_keywords as $tag_slug => $keywords) {
                    foreach ($keywords as $kw) {
                        if (strpos($title_lower, $kw) !== false || strpos($content_lower, $kw) !== false) {
                            $auto_tags[] = $tag_slug;
                            break;
                        }
                    }
                }
                if (!empty($auto_tags)) {
                    wp_set_object_terms($post_id, $auto_tags, 'pm_lesson_tag');
                }
            }

            // Meta
            update_post_meta($post_id, '_pm_lesson_order', $i + 1);
            update_post_meta($post_id, '_pm_lesson_duration', $lesson['duration']);
            update_post_meta($post_id, '_pm_lesson_difficulty', $lesson['difficulty']);
            update_post_meta($post_id, '_pm_lesson_xp', $lesson['xp']);
            update_post_meta($post_id, '_pm_lesson_has_quiz', '1');

            if (!empty($lesson['video'])) {
                update_post_meta($post_id, '_pm_lesson_video', $lesson['video']);
            }

            // Interactivity requirements
            if (!empty($lesson['interactivity'])) {
                update_post_meta($post_id, '_pm_lesson_interactivity', $lesson['interactivity']);
            }

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

    // ========================================
    // 4. Module Completion Requirements
    // ========================================
    update_option('pm_module_requirements', [
        'piano-discovery' => [
            'ear_trainer_questions' => 25,
            'description' => 'Answer 25 Ear Trainer questions correctly to complete this module',
        ],
        'expanding-range' => [
            'piano_hero_minutes' => 5,
            'ear_trainer_questions' => 30,
            'description' => 'Play 5 minutes of Piano Hero + answer 30 Ear Trainer questions',
        ],
        'reading-music' => [
            'ear_trainer_questions' => 35,
            'description' => 'Answer 35 Ear Trainer questions correctly to complete this module',
        ],
        'two-hands-together' => [
            'piano_hero_minutes' => 10,
            'description' => 'Play 10 minutes of Piano Hero practice to complete this module',
        ],
        'first-repertoire' => [
            'quiz_pass_rate' => 80,
            'description' => 'Pass all quizzes at 80%+ across all beginner modules to graduate',
        ],
    ]);

    // ========================================
    // 5. Module FAQ Content
    // ========================================
    $module_faqs = [
        'piano-discovery' => [
            ['q' => 'Do I need a full 88-key piano to start?', 'a' => 'No. A 61-key keyboard works perfectly for beginners. Even a 49-key keyboard will work for the first few modules.'],
            ['q' => 'How long should I practice each day?', 'a' => 'Start with 15-20 minutes daily. Consistent short sessions are much more effective than occasional long ones.'],
            ['q' => 'Should I learn notes before posture?', 'a' => 'Posture comes first. Good hand position prevents injury and makes everything easier as you progress.'],
            ['q' => 'Why start with C position?', 'a' => 'C position uses all white keys (no sharps or flats) and sits right in the middle of the keyboard, making it the easiest starting point.'],
            ['q' => 'What if I cannot reach Middle C?', 'a' => 'Adjust your bench position until Middle C is roughly aligned with the center of your body.'],
        ],
        'expanding-range' => [
            ['q' => 'What is the difference between C position and G position?', 'a' => 'C position places your thumb on C. G position places your thumb on G, four white keys to the right.'],
            ['q' => 'Do I need to memorize all 7 natural notes at once?', 'a' => 'No. Focus on C through G first, then add A and B. You already know C-D-E from Module 1.'],
            ['q' => 'What are chords and why do I need them?', 'a' => 'Chords are groups of 3+ notes played together. They form the harmonic foundation of nearly all Western music.'],
            ['q' => 'How fast should I play eighth notes?', 'a' => 'Start slowly, around 60 BPM. Speed comes naturally with accuracy. Never sacrifice correctness for speed.'],
            ['q' => 'Why learn Twinkle Twinkle Little Star?', 'a' => 'It uses both C and G positions, reinforcing hand shifting. It is also universally known, so you can hear if you make mistakes.'],
        ],
        'reading-music' => [
            ['q' => 'Why are there two clefs?', 'a' => 'The treble clef covers higher notes (typically right hand) and the bass clef covers lower notes (typically left hand). Together, they form the grand staff.'],
            ['q' => 'What are the mnemonics for remembering notes?', 'a' => 'Treble lines: Every Good Boy Does Fine (E-G-B-D-F). Treble spaces: FACE. Bass lines: Good Boys Do Fine Always. Bass spaces: All Cows Eat Grass.'],
            ['q' => 'Why does 4/4 time have 4 beats?', 'a' => 'The top number tells you how many beats per measure. The bottom number tells you which note value gets one beat. So 4/4 means 4 quarter-note beats per measure.'],
            ['q' => 'How do I get faster at reading notes?', 'a' => 'Practice with our Sight Reading app and Note Invaders game daily. Even 5 minutes a day makes a big difference over time.'],
            ['q' => 'What are ledger lines?', 'a' => 'Short lines added above or below the staff for notes too high or too low to fit. Middle C sits on a ledger line between the treble and bass clefs.'],
        ],
        'two-hands-together' => [
            ['q' => 'My left hand feels completely uncoordinated. Is this normal?', 'a' => 'Absolutely. Your dominant hand has years of fine motor training. The non-dominant hand needs patient, separate practice.'],
            ['q' => 'Should I practice hands separately first?', 'a' => 'Yes, always. Learn each hand\'s part individually until comfortable, then combine at a very slow tempo.'],
            ['q' => 'What is an Alberti bass?', 'a' => 'A broken chord pattern: bottom note, top note, middle note, top note (e.g., C-G-E-G). Widely used by Mozart and Classical-era composers.'],
            ['q' => 'How slow is slow enough when combining hands?', 'a' => 'Slow enough that you make zero mistakes. If you are making errors, slow down further. Speed is built on accuracy.'],
            ['q' => 'When will playing hands together feel natural?', 'a' => 'Most students feel comfortable after 2-4 weeks of daily practice. It becomes automatic over time.'],
        ],
        'first-repertoire' => [
            ['q' => 'What does piano and forte mean?', 'a' => 'Piano means soft and forte means loud. The full name of the instrument (pianoforte) means "soft-loud" in Italian.'],
            ['q' => 'How do I play louder without banging?', 'a' => 'Use arm weight rather than finger force. Drop your arm weight into the key with a firm but relaxed hand. The key still goes all the way down.'],
            ['q' => 'What is the difference between staccato and legato?', 'a' => 'Staccato means short and detached. Legato means smooth and connected. They are opposite articulations.'],
            ['q' => 'Am I ready to play real songs?', 'a' => 'If you have completed Modules 1-4 and passed all quizzes, you are ready for simplified arrangements of real pieces.'],
            ['q' => 'How do I build a daily practice routine?', 'a' => 'Start with 5 minutes of warm-ups, 10 minutes of technique, and 15 minutes of repertoire. Adjust as you progress.'],
        ],
    ];
    update_option('pm_module_faqs', $module_faqs);

    // ========================================
    // 6. Module Blog/Score Cross-References
    // ========================================
    $module_references = [
        'piano-discovery' => [
            'articles' => [
                ['title' => 'How to Sit Properly at the Piano', 'url' => '/explore/piano-learning-tutorials/beginner-lessons/how-to-sit-properly-at-the-piano/'],
                ['title' => 'Learn Piano Notes: A Complete Beginner Guide', 'url' => '/explore/piano-learning-tutorials/beginner-lessons/learn-piano-notes-a-complete-beginner-guide/'],
                ['title' => 'What is Middle C on Piano?', 'url' => '/explore/piano-learning-tutorials/beginner-lessons/what-is-middle-c-on-piano/'],
            ],
            'scores' => [
                ['title' => 'The First Term at the Piano', 'url' => '/listen-and-play/the-first-term-at-the-piano/'],
            ],
        ],
        'expanding-range' => [
            'articles' => [
                ['title' => 'Basic Piano Chords for Beginners', 'url' => '/explore/piano-learning-tutorials/beginner-lessons/basic-piano-chords-for-beginners/'],
                ['title' => 'Understanding Rhythm in Music', 'url' => '/explore/piano-learning-tutorials/technique-theory/understanding-rhythm-in-music/'],
            ],
            'scores' => [
                ['title' => 'Twinkle Twinkle Little Star', 'url' => '/listen-and-play/twinkle-twinkle-little-star/'],
                ['title' => 'The First Term at the Piano', 'url' => '/listen-and-play/the-first-term-at-the-piano/'],
            ],
        ],
        'reading-music' => [
            'articles' => [
                ['title' => 'How to Read Piano Sheet Music', 'url' => '/explore/piano-learning-tutorials/beginner-lessons/how-to-read-piano-sheet-music/'],
                ['title' => 'How to Improve Your Sight Reading Skills', 'url' => '/explore/piano-learning-tutorials/technique-theory/how-to-improve-your-sight-reading-skills/'],
                ['title' => 'Understanding Time Signatures', 'url' => '/explore/piano-learning-tutorials/technique-theory/understanding-time-signatures-in-piano-music/'],
            ],
            'scores' => [
                ['title' => 'The First Term at the Piano', 'url' => '/listen-and-play/the-first-term-at-the-piano/'],
                ['title' => '12 Melodious and Very Easy Studies, Op. 63', 'url' => '/listen-and-play/12-melodious-and-very-easy-studies-op-63/'],
            ],
        ],
        'two-hands-together' => [
            'articles' => [
                ['title' => 'How to Play with Both Hands Together', 'url' => '/explore/piano-learning-tutorials/beginner-lessons/how-to-play-with-both-hands-together-on-piano/'],
                ['title' => 'Developing Hand Independence', 'url' => '/explore/piano-learning-tutorials/technique-theory/developing-hand-independence-on-the-piano/'],
            ],
            'scores' => [
                ['title' => 'Ode to Joy', 'url' => '/listen-and-play/ode-to-joy/'],
                ['title' => 'The First Term at the Piano', 'url' => '/listen-and-play/the-first-term-at-the-piano/'],
            ],
        ],
        'first-repertoire' => [
            'articles' => [
                ['title' => 'How to Play Fur Elise on Piano', 'url' => '/explore/piano-learning-tutorials/song-tutorials/how-to-play-fur-elise-on-piano/'],
                ['title' => 'How to Build an Effective Piano Practice Routine', 'url' => '/explore/piano-learning-tutorials/practice-guides/how-to-build-an-effective-piano-practice-routine/'],
            ],
            'scores' => [
                ['title' => 'Fur Elise', 'url' => '/listen-and-play/fur-elise/'],
                ['title' => 'Let It Be', 'url' => '/listen-and-play/let-it-be/'],
            ],
        ],
    ];
    update_option('pm_module_references', $module_references);

    // ========================================
    // 7. Mark seed as done
    // ========================================
    update_option('pm_seed_content_version', $seed_version);
    update_option('pm_seed_lesson_ids', $created_ids);
}

// Hook it up - runs on admin_init
add_action('admin_init', 'pm_seed_content');