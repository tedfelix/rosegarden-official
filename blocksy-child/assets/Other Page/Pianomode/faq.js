/**
 * PianoMode FAQ — Data + Search + Tags + Accordion
 * SVG icons, multi-keyword search, 9 categories, ~200+ questions
 */

/* ═══════════════════════════════════════════
   SVG ICON MAP (gold outline style)
   ═══════════════════════════════════════════ */
const FAQ_ICONS = {
  'account-platform':      '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>',
  'getting-started':       '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M4.5 16.5c-1.5 1.26-2 5-2 5s3.74-.5 5-2c.71-.84.7-2.13-.09-2.91a2.18 2.18 0 0 0-2.91-.09z"/><path d="M12 15l-3-3a22 22 0 0 1 2-3.95A12.88 12.88 0 0 1 22 2c0 2.72-.78 7.5-6 11a22.35 22.35 0 0 1-4 2z"/><path d="M9 12H4s.55-3.03 2-4c1.62-1.08 3 0 3 0"/><path d="M12 15v5s3.03-.55 4-2c1.08-1.62 0-3 0-3"/></svg>',
  'reading-sheet-music':   '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/><path d="M8 7h8M8 11h6M8 15h4"/></svg>',
  'music-theory-harmony':  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><circle cx="8" cy="18" r="3"/><circle cx="18" cy="14" r="3"/><path d="M11 18V6l10-4v12"/></svg>',
  'technique-practice':    '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/><path d="M12 2v4"/><path d="M12 18v4"/><path d="M2 12h4"/><path d="M18 12h4"/></svg>',
  'sheet-music-scores':    '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="8" y1="13" x2="16" y2="13"/><line x1="8" y1="17" x2="16" y2="17"/></svg>',
  'interactive-games-tools':'<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M6 11h4"/><path d="M8 9v4"/><path d="M15 12h.01"/><path d="M18 10h.01"/><path d="M17.32 5H6.68a4 4 0 0 0-3.978 3.59c-.006.052-.01.101-.017.152C2.604 9.416 2 14.456 2 16a3 3 0 0 0 3 3c1 0 1.5-.5 2-1l1.414-1.414A2 2 0 0 1 9.828 16h4.344a2 2 0 0 1 1.414.586L17 18c.5.5 1 1 2 1a3 3 0 0 0 3-3c0-1.544-.604-6.584-.685-7.258-.007-.05-.011-.1-.017-.151A4 4 0 0 0 17.32 5z"/></svg>',
  'learning-lessons':      '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>',
  'digital-tools-technology':'<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><rect x="5" y="2" width="14" height="20" rx="2"/><path d="M15 7h-2a2 2 0 0 0-2 2v2a2 2 0 0 0 2 2h2"/><path d="M9 17h6"/><circle cx="12" cy="17" r=".5" fill="currentColor"/></svg>'
};

/* ═══════════════════════════════════════════
   FAQ DATA
   ═══════════════════════════════════════════ */
const FAQ_DATA = [
  {
    id: 'account-platform',
    title: 'Account & Platform',
    description: 'Manage your account, payments, and explore PianoMode features.',
    subcategories: [
      {
        id: 'account-management',
        title: 'Account Management',
        questions: [
          { q: 'How do I create a PianoMode account?', a: 'Click the "Sign Up" button on the top right corner of any page. You can register with your email address or sign in with your Google account. It only takes a few seconds.' },
          { q: 'How do I reset my password?', a: 'Go to the login page and click "Forgot Password." Enter your email and we will send you a reset link. Check your spam folder if you don\'t see it within a few minutes.' },
          { q: 'Can I delete my account?', a: 'Yes. Go to your Account dashboard, then Settings, and select "Delete Account." This action is permanent and will remove all your progress, saved scores, and personal data.' },
          { q: 'How do I update my profile information?', a: 'Visit your Account dashboard and click on your profile section. You can update your display name, avatar, email preferences, and notification settings from there.' },
          { q: 'Can I change the email address linked to my account?', a: 'Yes. Go to Account Settings and update your email. You will receive a confirmation link on both your old and new email addresses for security purposes.' },
          { q: 'Is my personal data safe on PianoMode?', a: 'Absolutely. We use industry-standard encryption (SSL/TLS) for all data in transit, and we never share your personal information with third parties. See our Privacy Policy for full details.' },
          { q: 'Can I use PianoMode on multiple devices?', a: 'Yes. Your account syncs across all devices. Log in on your phone, tablet, or computer and your progress, saved scores, and preferences will be there.' },
          { q: 'How do I enable dark mode?', a: 'PianoMode automatically follows your system preference (light or dark). On most devices, changing your OS appearance setting will switch PianoMode\'s theme instantly.' }
        ]
      },
      {
        id: 'subscription-payments',
        title: 'Subscription & Payments',
        questions: [
          { q: 'Is PianoMode free to use?', a: 'Yes! PianoMode offers a generous free tier with access to sheet music, articles, interactive games, the virtual piano studio, and beginner lessons. Premium features may be available in the future.' },
          { q: 'What payment methods do you accept?', a: 'We accept all major credit and debit cards (Visa, Mastercard, American Express) as well as PayPal. All payments are processed securely through encrypted connections.' },
          { q: 'How do I cancel my subscription?', a: 'Go to your Account dashboard, select "Subscription," and click "Cancel." Your access continues until the end of your current billing period. No cancellation fees apply.' },
          { q: 'Do you offer refunds?', a: 'Yes, we offer a 14-day money-back guarantee on all paid plans. Contact us through the Contact page within 14 days of purchase and we will process your refund promptly.' },
          { q: 'Will I lose my progress if my subscription expires?', a: 'No. Your progress, achievements, and saved scores are preserved even if your subscription lapses. You can pick up right where you left off when you resubscribe.' },
          { q: 'Are there student or educator discounts?', a: 'We are working on special pricing for students and music educators. Sign up for our newsletter to be the first to know when these plans become available.' },
          { q: 'Can I switch between subscription plans?', a: 'Yes. You can upgrade or downgrade your plan at any time from Account Settings. Changes take effect at the start of your next billing cycle.' },
          { q: 'Is there a free trial for premium features?', a: 'Yes — new users receive a trial period to explore all premium features. No credit card is required to start the trial.' }
        ]
      },
      {
        id: 'platform-features',
        title: 'Platform Features',
        questions: [
          { q: 'What features does PianoMode offer?', a: 'PianoMode is an all-in-one piano platform featuring a sheet music library with audio previews, interactive learning games (ear training, sight reading, note recognition), a virtual piano studio with recording capabilities, structured lessons, and a rich article library.' },
          { q: 'Can I track my learning progress?', a: 'Yes. Your Account dashboard shows your lesson progress, game scores, achievements, and practice streaks. You can see which levels you have completed and what comes next.' },
          { q: 'Is PianoMode available on mobile devices?', a: 'PianoMode is fully responsive and works on all modern browsers including mobile. The virtual piano, games, and lessons are all optimized for tablets. For the best experience on smaller screens, we recommend landscape orientation.' },
          { q: 'How do I contact support?', a: 'Visit our Contact Us page to send us a message. We typically respond within 24-48 hours. You can also browse this FAQ for instant answers to common questions.' },
          { q: 'Does PianoMode work offline?', a: 'Most features require an internet connection. However, if you download PDF sheet music to your device, you can use those scores offline anytime.' },
          { q: 'What browsers are supported?', a: 'PianoMode works on Chrome, Firefox, Safari, and Edge — all latest versions. For MIDI keyboard support in the Virtual Piano Studio, Chrome or Edge is recommended.' },
          { q: 'Can I share my progress on social media?', a: 'Yes. You can share achievements, badges, and game scores directly from your dashboard using the share buttons. Show your friends how far you have come!' },
          { q: 'How often is new content added?', a: 'We add new sheet music, articles, and lesson content on a regular basis. Follow us on social media or check the Explore page for the latest additions.' }
        ]
      }
    ]
  },
  {
    id: 'getting-started',
    title: 'Getting Started',
    description: 'Choose the right instrument, set up, and take your first steps.',
    subcategories: [
      {
        id: 'choosing-instrument',
        title: 'Choosing Your Instrument',
        questions: [
          { q: 'Is 61 keys enough to learn piano?', a: 'Yes, for starting out a 61-key keyboard covers the essentials — five full octaves for most beginner pieces, scales, chords, and exercises. However, you will eventually outgrow 61 keys for advanced classical repertoire. Plan to upgrade to 88 keys as you progress.' },
          { q: 'What about 22-key or 40-key keyboards for beginners?', a: '22-key keyboards are essentially toys, fine for toddlers but not for real learning. 40-key keyboards (about 3.5 octaves) can handle basic one-hand melodies but severely limit repertoire. For actual learning, move up to at least 61 keys as soon as possible.' },
          { q: 'Do smaller keyboards have any other issues?', a: 'Yes. Beyond limited range, most small keyboards lack weighted keys and pedal inputs, which limits technique development. Mini-sized keys can lead to poor hand posture, and cheap synthesized sounds don\'t prepare your ear for real piano tone.' },
          { q: 'How long can you learn on a small keyboard before needing to upgrade?', a: 'It varies. Some beginners spend 1-2 years on a 61-key model before needing more. If you notice you\'re limited in songs or exercises, or a teacher says you need more keys, that\'s your sign.' },
          { q: 'Does playing a weighted keyboard matter?', a: 'Absolutely. If you practice on non-weighted plastic keys, your fingers never develop the strength and control needed for a real acoustic piano. Weighted or semi-weighted keys are essential for building proper technique from the start.' },
          { q: 'What is the difference between weighted, semi-weighted, and unweighted keys?', a: 'Unweighted (synth-action) keys have no resistance — like pressing buttons. Semi-weighted keys add spring resistance for some feedback. Fully weighted (hammer-action) keys simulate a real acoustic piano mechanism. Hammer-action is best for serious learners.' },
          { q: 'Should I buy a digital piano or an acoustic piano?', a: 'For beginners, a quality digital piano (Yamaha, Roland, Kawai) is often the best choice: more affordable, never needs tuning, has headphone output for silent practice, and takes up less space. An acoustic piano offers unmatched touch and tone but requires more investment and maintenance.' },
          { q: 'What features should I look for in a beginner digital piano?', a: 'Look for 88 fully-weighted hammer-action keys, at least 64-note polyphony, a sustain pedal input, headphone jack, and a realistic piano sound. Brands like Yamaha P-series, Roland FP-series, and Kawai ES-series are excellent starting points.' },
          { q: 'Is a keyboard stand necessary?', a: 'Yes. Playing on a table or bed encourages bad posture. A proper X-stand or furniture-style stand positions the keyboard at the correct height for your seated posture, which is critical for technique and avoiding strain.' }
        ]
      },
      {
        id: 'essential-accessories',
        title: 'Essential Accessories',
        questions: [
          { q: 'Will music schools accept a 61-key keyboard for lessons?', a: 'Generally yes. Most teachers require at least 61 full-sized keys for home practice (88 strongly preferred). A 61-key keyboard is acceptable for starting. However, schools stress adding a sustain pedal as soon as possible — it\'s considered essential equipment.' },
          { q: 'Do I need a sustain pedal from the beginning?', a: 'It\'s highly recommended. A sustain pedal is used in nearly every piano piece and is essential for developing musicality. Get a standard damper-style pedal rather than a cheap switch-type pedal for a more realistic feel.' },
          { q: 'What kind of bench or seat should I use?', a: 'Use an adjustable piano bench or a sturdy chair that lets your elbows sit at keyboard height or slightly above. Avoid chairs that are too low or too high — proper seating height is critical for hand position and preventing injury.' },
          { q: 'Do I need headphones for practice?', a: 'Headphones are highly recommended for digital pianos, especially if you live with others or practice late at night. Use over-ear headphones with a flat frequency response for the most accurate sound representation.' },
          { q: 'Are music stands useful for keyboard players?', a: 'Yes. Many digital pianos have a built-in music rest, but a separate adjustable music stand gives you better angle control for reading sheet music and can hold tablets or printed scores at eye level.' },
          { q: 'What is a metronome and do I need one?', a: 'A metronome produces a steady click at a set tempo to help you keep time. It\'s absolutely essential for developing rhythmic accuracy. Most smartphones have free metronome apps, and many digital pianos have one built in.' },
          { q: 'Should I invest in a triple pedal unit?', a: 'Not immediately. Beginners only need a sustain (damper) pedal. As you advance to intermediate-level classical music, a soft (una corda) pedal and sostenuto pedal become useful. Triple pedal units come with furniture-style stands.' },
          { q: 'What cables or adapters do I need?', a: 'At minimum: a power adapter (usually included) and a sustain pedal cable. For connecting to a computer, a USB-B to USB-A cable handles both MIDI and audio on most modern digital pianos.' }
        ]
      },
      {
        id: 'first-steps',
        title: 'First Steps',
        questions: [
          { q: 'Can I learn piano without a teacher?', a: 'Yes! With apps, method books like Alfred\'s, and platforms like PianoMode, many beginners progress well on their own. However, a teacher provides invaluable feedback on posture and technique. Even a few lessons at the start can save years of frustration.' },
          { q: 'How long does it take to become fluent at the piano?', a: 'With 15-20 minutes of daily practice, you\'ll see noticeable progress in 2-3 months and solid fluency within 1-2 years depending on your consistency. Every practice session builds on the last.' },
          { q: 'Is it better to memorize music or read sheet music?', a: 'Both! They are complementary skills. Reading lets you learn new pieces quickly. Memorizing lets you focus on expression and performance. All great performers read the music first, then memorize it. Develop both skills in parallel.' },
          { q: 'What should I learn first as a complete beginner?', a: 'Start with proper hand position and posture, then learn the musical alphabet (A-G), find Middle C, and play simple five-finger patterns. PianoMode\'s beginner lessons guide you through this exact sequence step by step.' },
          { q: 'Am I too old to start learning piano?', a: 'Absolutely not. Adults often learn faster than children because they understand concepts more quickly and practice more deliberately. Many successful pianists started as adults. The best time to start is today.' },
          { q: 'How often should I practice as a beginner?', a: 'Aim for 15-30 minutes daily. Short, focused daily sessions are far more effective than occasional marathon practices. Consistency is the single most important factor for progress.' },
          { q: 'Should I learn with my right hand or left hand first?', a: 'Start with your right hand in the C position (thumb on Middle C). Most beginner method books begin with the right hand because the treble clef melody is easiest to hear and follow. Left hand comes soon after, then hands together.' },
          { q: 'What is the best age to start piano lessons?', a: 'Children can start as young as 5-6 years old, though 7-8 is ideal for focused learning. There is no upper age limit — adults in their 60s, 70s, and beyond successfully learn piano every day.' },
          { q: 'Do I need to learn music theory to play piano?', a: 'Basic theory (note names, rhythms, key signatures) is essential and learned naturally through lessons. You don\'t need to master advanced theory before playing, but understanding fundamentals dramatically accelerates your progress.' }
        ]
      }
    ]
  },
  {
    id: 'reading-sheet-music',
    title: 'Reading Sheet Music',
    description: 'Learn to read notation, understand the staff, clefs, and time signatures.',
    subcategories: [
      {
        id: 'learning-to-read',
        title: 'Learning to Read',
        questions: [
          { q: 'Can I learn to read sheet music without a teacher?', a: 'Yes. With a method book (like Alfred\'s), a practice app, and PianoMode\'s structured lessons, you can teach yourself. A teacher provides crucial feedback on reading habits — a few early lessons can save years of frustration.' },
          { q: 'How long does it take to read piano sheet music fluently?', a: 'With consistent daily practice (even 15-20 minutes), you\'ll see noticeable progress in 2-3 months. True fluency — sight-reading complex pieces — develops over 1-2 years of regular practice.' },
          { q: 'What is the hardest part about learning to read piano music?', a: 'For most beginners, it\'s rhythm and hand independence — not the notes. Your brain struggles to make one hand play a slow half note while the other plays fast eighth notes. The solution: practice hands separately and use a metronome.' },
          { q: 'What is sight reading?', a: 'Sight reading is playing a piece of music you have never seen before, in real time. It combines note reading, rhythm, dynamics, and hand coordination simultaneously. It\'s a skill that improves dramatically with daily practice on unfamiliar material.' },
          { q: 'Should I look at my hands while playing?', a: 'Try to minimize looking at your hands. Developing the ability to play "by feel" (proprioception) lets you keep your eyes on the music. Use occasional glances for large jumps, but practice keeping your eyes on the score as much as possible.' },
          { q: 'What are ledger lines?', a: 'Ledger lines are short horizontal lines added above or below the staff to extend its range. Middle C, for example, sits on a ledger line between the treble and bass staves. They let you write notes that go beyond the five staff lines.' },
          { q: 'How do I read two staves at the same time?', a: 'Start by reading each hand separately until comfortable. Then combine slowly. Your eyes should scan slightly ahead of where you\'re playing. With practice, reading two staves becomes automatic — like reading words instead of individual letters.' },
          { q: 'What are the best books for learning to read music?', a: 'Popular choices include Alfred\'s Adult All-in-One Course, Faber Piano Adventures (for younger learners), and Bastien Piano Basics. Each combines note reading with theory and repertoire progressively.' }
        ]
      },
      {
        id: 'staff-clefs',
        title: 'Staff & Clefs',
        questions: [
          { q: 'How many lines does a musical staff have?', a: 'A musical staff has 5 horizontal lines. Notes are placed on the lines and in the spaces between them. Additional notes above or below the staff use short temporary lines called ledger lines.' },
          { q: 'What is a grand staff?', a: 'A grand staff combines two staves (treble and bass) connected by a brace on the left. The top staff (treble clef) typically covers the right hand, and the bottom staff (bass clef) covers the left hand.' },
          { q: 'Where is Middle C on the grand staff?', a: 'Middle C sits on a ledger line between the treble and bass staves. It can be written just below the treble staff or just above the bass staff — both represent the same note (C4).' },
          { q: 'What do the treble clef line notes spell?', a: 'From bottom to top: E-G-B-D-F. The mnemonic is "Every Good Boy Does Fine." The spaces spell F-A-C-E, which conveniently spells the word FACE.' },
          { q: 'What do the bass clef line notes spell?', a: 'From bottom to top: G-B-D-F-A ("Good Boys Do Fine Always"). The spaces spell A-C-E-G ("All Cows Eat Grass"). The bass clef is also called the F clef.' },
          { q: 'What is an octave marking (8va)?', a: '8va above notes means play one octave higher than written. 8vb below notes means play one octave lower. These markings avoid excessive ledger lines and keep the music readable.' },
          { q: 'Why does piano use two clefs?', a: 'The piano\'s wide range (88 keys, over 7 octaves) requires two clefs. The treble clef covers the upper range (right hand), the bass clef covers the lower range (left hand). Together they form the grand staff.' },
          { q: 'What is a key signature?', a: 'A key signature is the set of sharps or flats placed at the beginning of each staff line. It tells you which notes are consistently sharp or flat throughout the piece, identifying the musical key (e.g., one sharp = G major or E minor).' }
        ]
      },
      {
        id: 'time-signatures',
        title: 'Time Signatures & Rhythm',
        questions: [
          { q: 'What is the "C" symbol at the start of the staff?', a: 'The "C" stands for Common Time, exactly the same as 4/4 time. If the "C" has a vertical line through it, it\'s Cut Time (Alla Breve), which is 2/2 time.' },
          { q: 'Can a time signature change the tempo?', a: 'No. The time signature changes the grouping of notes (beats per measure), not the speed. However, composers often pair a time signature change with a tempo change.' },
          { q: 'Why do some pieces have no time signature?', a: 'This is called "Free Time" or unmeasured music. It\'s found in Gregorian chant, some contemporary music, and cadenzas. The rhythm follows the performer\'s discretion.' },
          { q: 'What is the hardest time signature to play?', a: 'Many pianists find constantly changing meters (like Stravinsky\'s Rite of Spring) or unusual odd meters (7/8, 11/8) the most challenging because they prevent settling into a rhythmic groove.' },
          { q: 'What is the difference between 3/4 and 6/8 time?', a: '3/4 has three quarter-note beats per measure (ONE-two-three). 6/8 has two dotted-quarter beats with three eighth notes each (ONE-two-three-FOUR-five-six). The "feel" is different even though both have six eighth notes per measure.' },
          { q: 'What does a dot after a note mean?', a: 'A dot adds half the note\'s value to its duration. A dotted half note = 3 beats (2 + 1). A dotted quarter note = 1.5 beats (1 + 0.5). It\'s one of the most common rhythmic devices in music.' },
          { q: 'What is syncopation?', a: 'Syncopation places emphasis on normally weak beats or between beats, creating rhythmic surprise and energy. It\'s a fundamental element of jazz, pop, Latin, and many other styles.' },
          { q: 'How do I count triplets?', a: 'Triplets divide a beat into three equal parts instead of two. Count them as "1-trip-let, 2-trip-let" or "1-and-a, 2-and-a." Practice them slowly with a metronome, making all three notes perfectly even.' }
        ]
      }
    ]
  },
  {
    id: 'music-theory-harmony',
    title: 'Music Theory & Harmony',
    description: 'Understand chords, scales, progressions, and how music works.',
    subcategories: [
      {
        id: 'chords-progressions',
        title: 'Chords & Progressions',
        questions: [
          { q: 'What is a "Spy Chord"?', a: 'The Minor Major Seventh chord (e.g. C-Eb-G-B) is nicknamed the "Spy Chord" or "Hitchcock Chord." It combines the darkness of a minor triad with the tension of a major seventh, creating a mysterious sound iconic in film noir.' },
          { q: 'What is a Tritone Substitution?', a: 'In jazz, a Tritone Substitution replaces a Dominant 7th chord with another Dominant 7th whose root is a tritone (6 semitones) away. Example: replacing G7 with Db7 creates a smooth chromatic bass line: Dm7 to Db7 to Cmaj7.' },
          { q: 'Why does a Major Chord sound "happy"?', a: 'The intervals of a major triad align closely with the natural harmonics of a vibrating string, making them highly consonant and perceived as "stable" or "bright" by the human ear.' },
          { q: 'Is it necessary to learn Classical Harmony if I play Pop?', a: 'Yes! Pop harmony is essentially simplified tonal harmony. Understanding chord functions, voice leading, and cadences applies to every genre and lets you break the rules creatively.' },
          { q: 'What is Voice Leading?', a: 'Voice leading is moving from one chord to another smoothly by moving individual notes (voices) the smallest interval possible. This is what makes professional pianists sound fluid and connected.' },
          { q: 'What is a chord inversion?', a: 'An inversion rearranges which note of the chord is lowest. Root position has the root at the bottom, first inversion has the 3rd, second inversion has the 5th. Inversions create smoother bass lines and better voice leading.' },
          { q: 'What are the most common chord progressions in pop music?', a: 'The I-V-vi-IV progression (C-G-Am-F in C major) is the most common, used in hundreds of hit songs. Other popular ones include I-vi-IV-V, vi-IV-I-V, and the 12-bar blues (I-I-I-I-IV-IV-I-I-V-IV-I-V).' },
          { q: 'What is the difference between major and minor chords?', a: 'A major chord (root-major 3rd-5th) sounds bright and stable. A minor chord (root-minor 3rd-5th) sounds darker and more melancholic. The only difference is the middle note — lowered by one semitone in minor.' },
          { q: 'What is a seventh chord?', a: 'A seventh chord adds a note a seventh above the root to a basic triad. Common types: Major 7th (dreamy), Dominant 7th (bluesy, tense), Minor 7th (mellow), and Diminished 7th (dramatic, unstable). They add harmonic richness beyond basic triads.' }
        ]
      },
      {
        id: 'scales-keys',
        title: 'Scales & Keys',
        questions: [
          { q: 'What is the Circle of Fifths?', a: 'A visual map showing all 12 major and minor keys arranged by fifths. Moving clockwise adds sharps; counterclockwise adds flats. Essential for understanding key signatures, chord relationships, and modulation.' },
          { q: 'What is the difference between major and minor scales?', a: 'A major scale (W-W-H-W-W-W-H) sounds bright. A minor scale (W-H-W-W-H-W-W) sounds darker. The key difference is the lowered third note in minor, which changes the entire mood.' },
          { q: 'How many musical keys are there?', a: 'There are 12 major keys and 12 minor keys (24 total). Each major key has a relative minor sharing the same key signature — C major and A minor both have no sharps or flats.' },
          { q: 'What are modes in music?', a: 'Modes are scales derived from the major scale starting on different degrees. The 7 modes (Ionian, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian) each have a unique character. Dorian sounds jazzy, Lydian sounds dreamy.' },
          { q: 'What is a pentatonic scale?', a: 'A five-note scale that omits the "tension" notes of a full scale. The major pentatonic (1-2-3-5-6) is common in pop and country; the minor pentatonic (1-b3-4-5-b7) dominates blues and rock. Great for improvisation because every note sounds good.' },
          { q: 'What is a chromatic scale?', a: 'A scale containing all 12 notes, each a semitone apart. It\'s not a "key" — it\'s used for runs, embellishments, and exercises. Playing all 12 notes up and down the keyboard builds finger dexterity and familiarity with black keys.' },
          { q: 'What does "relative minor" mean?', a: 'Every major key has a relative minor that shares the same key signature but starts on the 6th degree. C major\'s relative minor is A minor. They use the same notes but have a completely different mood and tonal center.' },
          { q: 'How do I know what key a piece is in?', a: 'Check the key signature (sharps or flats at the beginning) and look at the first and last notes/chords — they usually indicate the tonic. The last chord of a piece almost always confirms the key.' }
        ]
      },
      {
        id: 'advanced-harmony',
        title: 'Advanced Concepts',
        questions: [
          { q: 'What are Shell Voicings in jazz?', a: 'Shell voicings are jazz chords with only root, 3rd, and 7th — no 5th. Popularized in Bebop, they outline harmonic function clearly while leaving space for extensions, bass lines, and melodies.' },
          { q: 'What is Modal Interchange?', a: 'Using chords borrowed from a parallel scale. For example, playing Ab major (from C minor) while in C major adds unexpected color without fully changing key. Common in film scores and pop ballads.' },
          { q: 'What is a ii-V-I progression?', a: 'The most important chord progression in jazz: Dm7-G7-Cmaj7 in C major. The roots descend by perfect fifths, creating the strongest possible harmonic resolution. Mastering this in all 12 keys is fundamental to jazz piano.' },
          { q: 'What is a secondary dominant?', a: 'A secondary dominant is a V7 chord that resolves to a chord other than the tonic. For example, D7 resolving to G in the key of C major (D7 is the "V of V"). They add brief moments of tension and color.' },
          { q: 'What is modulation?', a: 'Modulation is changing from one key to another within a piece. Common techniques include using a pivot chord (shared between both keys), a chromatic modulation, or a direct key change. It adds emotional development and contrast.' },
          { q: 'What is a diminished chord used for?', a: 'Diminished chords create tension and instability. The fully diminished seventh chord (e.g. Bdim7) is symmetrical — it divides the octave into four equal parts. It\'s used as a passing chord, a dominant substitute, or to create dramatic suspense.' },
          { q: 'What is an augmented chord?', a: 'An augmented chord raises the fifth by a semitone (e.g. C-E-G#). It sounds unresolved and dreamlike. Common in jazz, film music, and as a passing chord between major chords in Beatles-style progressions.' },
          { q: 'What is a sus chord?', a: 'A suspended chord replaces the 3rd with either the 4th (sus4) or 2nd (sus2). It creates a floating, unresolved sound that wants to "resolve" back to the major or minor chord. Very common in pop and worship music.' }
        ]
      }
    ]
  },
  {
    id: 'technique-practice',
    title: 'Technique & Practice',
    description: 'Hand position, finger independence, practice routines, and exercises.',
    subcategories: [
      {
        id: 'hand-position-posture',
        title: 'Hand Position & Posture',
        questions: [
          { q: 'Where should your elbows be when sitting at the piano?', a: 'At keyboard height or slightly above. This ensures proper alignment from shoulder to fingertip. Too low causes wrist tension; too high causes you to press down with arm weight rather than finger control.' },
          { q: 'What is a common beginner mistake related to hand shape?', a: 'Collapsing knuckles — the first knuckle caves inward instead of maintaining a natural curve. This weakens tone and can lead to injury. Imagine holding a small ball: that curve is your ideal hand shape.' },
          { q: 'How should I sit at the piano?', a: 'Sit at the front half of the bench, feet flat on the floor. Forearms roughly parallel to the floor. Back straight but relaxed. You should reach both ends of the keyboard by leaning slightly, not stretching.' },
          { q: 'What is the correct wrist position?', a: 'Your wrists should be level with your forearms — not drooping below the keys or arching high above them. A neutral wrist allows the fingers to curve naturally and move freely without tension.' },
          { q: 'How far should I sit from the piano?', a: 'About an arm\'s length away. When your hands are on the keys, your elbows should be slightly in front of your body, not pinned to your sides. You need room to move laterally for passages that span the keyboard.' },
          { q: 'What should I do if I feel tension or pain while playing?', a: 'Stop immediately. Shake out your hands, roll your shoulders, and take a break. Tension usually comes from poor posture, flat fingers, or playing too long without rest. If pain persists, consult a teacher or a physiotherapist.' },
          { q: 'Should I use arm weight or finger strength to press keys?', a: 'Modern piano pedagogy emphasizes arm weight transfer rather than isolated finger strength. Let the natural weight of your relaxed arm flow through your fingers. This produces a richer tone and reduces injury risk.' },
          { q: 'How do I avoid shoulder tension while playing?', a: 'Consciously drop your shoulders before you start playing. Check them periodically — they tend to creep upward without you noticing. Take a deep breath and release the shoulders every few minutes during practice.' }
        ]
      },
      {
        id: 'finger-independence',
        title: 'Finger Independence',
        questions: [
          { q: 'Why is my 4th finger so weak?', a: 'It\'s anatomical: the 4th finger (ring finger) shares tendons with the 3rd and 5th fingers. You\'re fighting biology. Targeted independence exercises — slowly lifting and pressing each finger individually — are the only effective solution.' },
          { q: 'Can I build finger strength without a piano?', a: 'Yes, through isometric exercises on a flat surface and finger-specific resistance tools. However, you must eventually practice on real keys to translate raw strength into musical touch and control.' },
          { q: 'Can you do Hanon exercises on a 40-key keyboard?', a: 'Most early Hanon exercises cover 1-2 octaves and will fit. Later exercises spanning 3+ octaves need transposing. The core benefit — finger movement and repetition — can still be achieved by adapting the range.' },
          { q: 'What are the best exercises for finger independence?', a: 'Hanon exercises (especially 1-20), Czerny studies, five-finger patterns in all keys, and simple trills between weak finger pairs (3-4, 4-5). Practice slowly and evenly — speed comes naturally with accuracy.' },
          { q: 'How long does it take to develop good finger independence?', a: 'Noticeable improvement comes within 2-4 weeks of daily targeted exercises. True independence (playing complex counterpoint fluently) develops over months and years. It\'s a gradual, ongoing process.' },
          { q: 'What is the thumb-under technique?', a: 'When playing scales and arpeggios, the thumb passes under the hand to the next key position, allowing smooth, continuous runs. It\'s one of the most important technical skills to master early. Practice slowly to keep the hand stable.' },
          { q: 'How do I play trills evenly?', a: 'Start very slowly with even rhythm, gradually increase speed. Use finger pairs 2-3 or 1-2 for the strongest trills. Keep your hand relaxed and use small, light finger movements from the knuckle joint, not the whole hand.' },
          { q: 'Should I practice scales every day?', a: 'Yes. Scales build finger strength, independence, key familiarity, and keyboard geography. Practice all major and minor scales, hands separately then together, at a comfortable tempo with a metronome. It\'s the foundation of piano technique.' }
        ]
      },
      {
        id: 'practice-routines',
        title: 'Practice Routines',
        questions: [
          { q: 'How many hours a day should I practice technique?', a: 'For most students, 15-30 minutes of dedicated technical work (scales, arpeggios, Hanon) is sufficient. The rest of your time should go to repertoire, sight reading, and ear training. Quality matters more than hours.' },
          { q: 'How much time should I practice each day?', a: '15-30 minutes daily for beginners. Consistency matters more than duration. Short, focused daily sessions are far more effective than occasional long marathons. As you advance, gradually increase to 45-60 minutes.' },
          { q: 'At what speed should I practice?', a: 'Start at 60 BPM with quarter notes. Only increase the tempo when you can play a passage 10 times perfectly in a row. Slow practice builds accuracy and muscle memory. Speed is a byproduct of precision.' },
          { q: 'How should I structure a practice session?', a: 'A balanced 30-minute session: 5 min warm-up (scales/arpeggios), 10 min technical work on difficult passages, 10 min repertoire, 5 min sight reading or ear training. Adjust proportions based on your goals and level.' },
          { q: 'What is the best time of day to practice?', a: 'Whenever you can be most consistent. Morning practice benefits from a fresh mind, but any regular time works. The key is building a habit — same time, same duration, every day.' },
          { q: 'Should I practice hands separately or together?', a: 'Both! Always learn new passages hands separately first until each hand is secure and fluent. Then combine slowly. Hands-separate practice is not just for beginners — professionals do it routinely when learning new pieces.' },
          { q: 'How do I stay motivated to practice?', a: 'Set small, achievable goals for each session. Track your progress. Mix challenging exercises with pieces you enjoy. Use PianoMode\'s games as fun warm-ups. Remember: even 10 minutes of focused practice moves you forward.' },
          { q: 'What is deliberate practice?', a: 'Deliberate practice means focusing on specific weaknesses rather than playing through pieces start to finish. Isolate the hard measures, practice them slowly, analyze what makes them difficult, and repeat until mastered. It\'s the fastest path to improvement.' }
        ]
      },
      {
        id: 'arpeggios-exercises',
        title: 'Arpeggios & Exercises',
        questions: [
          { q: 'How can I stop my arpeggios from sounding "choppy"?', a: 'Use "syncopated pedaling" — change the pedal just after playing the first note of each new chord, not before. Also practice the thumb-under crossings slowly for fluid motion.' },
          { q: 'What are "Cross-Hand" arpeggios?', a: 'When the left hand crosses over the right (or vice versa) to reach a high or low note. Common in works by Scarlatti and Liszt, adding visual flair and sonic range to the performance.' },
          { q: 'What is the Alberti Bass pattern?', a: 'A broken-chord accompaniment played bottom-top-middle-top. Widely used in Classical Era music (Mozart, Haydn) to create gentle rhythmic movement in the left hand while the right hand plays the melody.' },
          { q: 'How fast should I be able to play scales?', a: 'A good intermediate goal is 4 octaves in sixteenth notes at 80-100 BPM. Advanced players aim for 120+ BPM. But accuracy, evenness, and clean tone matter far more than raw speed — never sacrifice quality for tempo.' },
          { q: 'What is a Czerny exercise?', a: 'Carl Czerny wrote hundreds of progressive piano exercises (Op. 299, Op. 740, etc.) focusing on specific technical challenges: rapid scales, arpeggios, trills, octaves, and leaps. They bridge the gap between exercises and real repertoire.' },
          { q: 'How do I practice octaves without fatigue?', a: 'Use wrist rotation and arm weight rather than squeezing with your hand. Keep your hand relaxed with a slight arch. Practice short bursts with rests. Octave technique is about efficient motion, not brute force.' },
          { q: 'What are broken chords?', a: 'Playing the notes of a chord one at a time instead of simultaneously. Arpeggios, Alberti bass, and rolled chords are all forms of broken chords. They create movement and texture from static harmony.' },
          { q: 'Should I practice with a metronome?', a: 'Yes, regularly. A metronome develops your internal sense of pulse and exposes rhythmic inconsistencies you might not hear otherwise. Start slow, and only increase tempo when the passage is perfectly even.' }
        ]
      }
    ]
  },
  {
    id: 'sheet-music-scores',
    title: 'Sheet Music & Scores',
    description: 'Browse, download, and understand our sheet music library.',
    subcategories: [
      {
        id: 'pianomode-library',
        title: 'PianoMode Library',
        questions: [
          { q: 'How do I download sheet music from PianoMode?', a: 'Visit the Listen & Play page, browse or search for a score, then click on it. Each score page includes a PDF download button. You can also preview the audio before downloading.' },
          { q: 'Are the sheet music scores free?', a: 'Yes, all sheet music in the PianoMode library is free to download as PDF. Each score includes an audio preview so you can listen before downloading. Our library grows continuously.' },
          { q: 'What difficulty levels are available?', a: 'Scores range from Beginner (simple melodies, basic rhythms) through Intermediate (hand independence, varied keys) to Advanced (complex rhythms, wide range, demanding technique). Each score is clearly labeled.' },
          { q: 'What format are the scores in?', a: 'All sheet music is provided as high-quality PDF files, compatible with any device or printer. PDFs display beautifully on tablets for paperless practice at the piano.' },
          { q: 'Can I print the sheet music?', a: 'Absolutely. All PDF scores are formatted for standard paper sizes (A4 and Letter). Print them at home or at a copy shop. The formatting is optimized for clean, readable printing.' },
          { q: 'How do I find a specific piece?', a: 'Use the search bar on the Listen & Play page to search by title, composer, or style. You can also filter by difficulty level, musical style, or composer to narrow results.' },
          { q: 'Can I request a specific piece to be added?', a: 'Yes! Use the Contact Us page to submit requests. We prioritize pieces that are frequently requested and fill gaps in our difficulty level and style coverage.' },
          { q: 'Do scores include fingering suggestions?', a: 'Many of our scores include fingering annotations, especially at the beginner and intermediate levels. Advanced scores typically leave fingering to the performer\'s discretion, as is standard in professional editions.' }
        ]
      },
      {
        id: 'difficulty-progression',
        title: 'Difficulty & Progression',
        questions: [
          { q: 'Can I skip difficulty levels if I already have experience?', a: 'Yes! Take our placement assessment from the Learn page to find the right starting point. No need to start from scratch if you already have a foundation.' },
          { q: 'How are scores categorized?', a: 'By composer, difficulty level, and musical style. Each score also shows the key signature, time signature, and approximate duration so you know exactly what to expect.' },
          { q: 'What makes a piece "beginner" vs "intermediate"?', a: 'Beginner pieces use limited hand positions, simple rhythms, no key changes, and mostly stepwise motion. Intermediate pieces add hand independence, varied articulation, key changes, wider range, and more complex rhythmic patterns.' },
          { q: 'How do I know when I am ready for the next difficulty level?', a: 'When you can play pieces at your current level with comfort, accuracy, and musical expression — not just "getting through the notes." If current pieces feel easy and you\'re sight-reading them quickly, it\'s time to move up.' },
          { q: 'Are there simplified arrangements of famous pieces?', a: 'Yes. We offer arrangements of well-known classical and popular pieces adapted for various skill levels. A beginner can enjoy a simplified Fur Elise while an advanced player tackles the original version.' },
          { q: 'What is the ABRSM grading system?', a: 'The ABRSM (Associated Board of the Royal Schools of Music) uses grades 1-8 plus diploma levels. Grade 1-2 is beginner, 3-5 is intermediate, 6-8 is advanced. Our difficulty labels roughly correspond to these standard levels.' },
          { q: 'Should I play pieces above my level to challenge myself?', a: 'Occasionally attempting one level above is good motivation, but don\'t make it your main practice. Playing pieces too far above your level creates bad habits (tension, wrong notes, poor rhythm). Most of your practice should be at or slightly below your level.' },
          { q: 'How many pieces should I be working on at once?', a: 'Typically 3-5 pieces at different stages: one you\'re polishing for performance, two you\'re actively learning, and one or two for sight-reading practice. This keeps practice varied and avoids boredom.' }
        ]
      }
    ]
  },
  {
    id: 'interactive-games-tools',
    title: 'Interactive Games & Tools',
    description: 'Explore our games, virtual piano studio, and training tools.',
    subcategories: [
      {
        id: 'games-overview',
        title: 'Games Overview',
        questions: [
          { q: 'What games does PianoMode offer?', a: 'PianoMode offers several interactive music games: Ear Trainer (interval and chord recognition), Note Invaders (arcade-style note reading), Piano Hero (rhythm and timing), Ledger Line Legend, Sight Reading Trainer, and Melody Merge. Each targets a specific skill.' },
          { q: 'Do the games actually help improve piano skills?', a: 'Yes! Each game targets a specific musical skill — note reading, ear training, rhythm accuracy, or sight reading. Regular play builds pattern recognition and reflexes that directly transfer to your piano playing.' },
          { q: 'Are the games free to play?', a: 'Yes, all PianoMode games are completely free. No account required to play, though creating one lets you track high scores, earn achievements, and monitor progress over time.' },
          { q: 'Can I play the games on my phone or tablet?', a: 'Yes, all games are responsive and work on mobile browsers. For the best experience, we recommend a tablet in landscape mode. Some games benefit from a larger screen.' },
          { q: 'What is the Ear Trainer game?', a: 'The Ear Trainer plays intervals, chords, or melodies and asks you to identify them. It trains your ability to recognize musical sounds by ear — essential for playing by ear, improvising, and understanding music deeply.' },
          { q: 'What is Note Invaders?', a: 'An arcade-style game where notes fall down the screen and you must identify them before they reach the bottom. It builds rapid note recognition in both treble and bass clef through fun, fast-paced gameplay.' },
          { q: 'Do games track my progress over time?', a: 'Yes. When logged in, your game scores, streaks, and achievements are saved. You can see your improvement over time on your Account dashboard and compare your best scores.' },
          { q: 'Can games replace traditional practice?', a: 'Games are a powerful supplement but not a full replacement. They excel at building specific skills (note reading speed, ear recognition) in an engaging way. Combine them with repertoire practice, technique work, and lessons for balanced progress.' }
        ]
      },
      {
        id: 'virtual-piano-studio',
        title: 'Virtual Piano Studio',
        questions: [
          { q: 'What is the Virtual Piano Studio?', a: 'A full-featured online instrument and recording environment: 88-key virtual piano, multi-track DAW, 16-step drum machine, microphone recording, audio effects (reverb, delay), and WAV/MIDI export — all in your browser.' },
          { q: 'Can I record my playing in the Virtual Piano?', a: 'Yes. The studio supports multi-track recording: lay down a piano track, add drum beats, record your voice, and mix everything together. Export as WAV or MIDI.' },
          { q: 'Does the Virtual Piano support MIDI keyboards?', a: 'Yes! Connect any USB MIDI keyboard and the Virtual Piano detects it via the Web MIDI API (Chrome and Edge). This lets you play with real weighted keys while using the studio\'s recording and effects.' },
          { q: 'Can I upload my own audio tracks?', a: 'Yes. The DAW section lets you upload audio files (backing tracks, vocals, etc.) and layer them with virtual instruments. Great for practicing with accompaniment or building arrangements.' },
          { q: 'Is the Virtual Piano suitable for practicing?', a: 'It\'s excellent for theory exploration, composition, and ear training. For serious technique practice, a physical keyboard with weighted keys is necessary. The virtual piano is best as a creative tool and quick reference.' },
          { q: 'What audio effects are available?', a: 'The studio includes reverb (room, hall, plate), delay, and basic EQ. These let you shape your sound for recording or simply make practice more enjoyable with a richer, more realistic piano tone.' },
          { q: 'Can I use the Virtual Piano on iPad?', a: 'Yes. The Virtual Piano works on iPad browsers. Touch the on-screen keys to play, or connect an external MIDI keyboard via USB-C for a more natural playing experience.' },
          { q: 'How do I export my recordings?', a: 'Click the Export button in the DAW section to download your project as a WAV audio file or MIDI data. WAV files can be shared directly; MIDI files can be imported into professional DAWs like Logic Pro or Ableton.' }
        ]
      },
      {
        id: 'ear-training-sight-reading',
        title: 'Ear Training & Sight Reading',
        questions: [
          { q: 'What is ear training and why does it matter?', a: 'Ear training develops your ability to identify notes, intervals, chords, and rhythms by hearing. It\'s essential for playing by ear, improvising, tuning, and understanding music deeply.' },
          { q: 'How can I improve my sight reading?', a: 'Practice reading new, unfamiliar music every day — even simple pieces. Focus on reading ahead (looking at the next measure while playing the current one). PianoMode\'s Sight Reading Trainer generates random exercises at your level.' },
          { q: 'What is the difference between sight reading and note reading?', a: 'Note reading is identifying individual notes on the staff. Sight reading is playing an entire unfamiliar piece in real time — combining note reading with rhythm, dynamics, and hand coordination simultaneously.' },
          { q: 'How often should I train my ear?', a: 'Even 5-10 minutes daily makes a significant difference. Interval recognition, chord quality identification, and melodic dictation all improve rapidly with consistent short sessions. Use the Ear Trainer game for an engaging daily routine.' },
          { q: 'Can I develop perfect pitch as an adult?', a: 'True "absolute" pitch is extremely rare and typically develops in early childhood. However, adults can develop excellent "relative pitch" — the ability to identify intervals and notes by their relationship to a reference note. This is equally useful in practice.' },
          { q: 'What intervals should I learn to recognize first?', a: 'Start with perfect octave, perfect fifth, and major/minor thirds. Then add perfect fourth, major/minor seconds, and tritone. Associate each interval with a familiar song opening (e.g., a perfect fifth sounds like the Star Wars theme).' },
          { q: 'How do I practice rhythmic reading?', a: 'Clap or tap rhythms before playing them on the piano. Use a metronome and count aloud. Start with simple patterns (quarter and half notes) and progressively add eighth notes, dotted rhythms, syncopation, and triplets.' },
          { q: 'What is melodic dictation?', a: 'Listening to a melody and writing it down in music notation. It combines ear training with music theory knowledge. Start with simple 4-8 note melodies in a familiar key and gradually increase complexity.' }
        ]
      }
    ]
  },
  {
    id: 'learning-lessons',
    title: 'Learning & Lessons',
    description: 'Curriculum, lesson structure, and how to progress effectively.',
    subcategories: [
      {
        id: 'curriculum',
        title: 'Curriculum & Structure',
        questions: [
          { q: 'Do I need a real piano or keyboard to start lessons?', a: 'While you can learn theory concepts without one, we strongly recommend at least a 61-key keyboard. A weighted or semi-weighted keyboard helps develop proper technique from the start.' },
          { q: 'Are the lessons suitable for children?', a: 'Our curriculum is designed for learners aged 12 and up. Younger children may need parental guidance for theory concepts, but the interactive exercises and games are engaging for all ages.' },
          { q: 'Will I learn to read sheet music in the lessons?', a: 'Absolutely. Reading music is integrated throughout the curriculum from the Beginner level. You\'ll progressively learn treble and bass clef, rhythm notation, key signatures, dynamics, and more.' },
          { q: 'What styles of music will I learn?', a: 'The curriculum covers classical foundations, pop chord progressions, and introduces jazz at higher levels. You\'ll play real pieces from various genres as you progress through the modules.' },
          { q: 'How are lessons structured?', a: 'Each lesson includes a video or text explanation, interactive practice exercises, a mini piano for trying concepts, and a quiz to test understanding. Lessons are grouped into progressive modules.' },
          { q: 'Is there a placement test?', a: 'Yes. The placement assessment on the Learn page evaluates your reading, rhythm, and playing skills to recommend the best starting level. It takes about 10 minutes and helps you skip content you already know.' },
          { q: 'Are there quizzes or tests?', a: 'Each lesson includes interactive quizzes with detailed explanations for each answer. These reinforce learning and help identify areas that need review. You must pass the quiz to unlock the next lesson.' },
          { q: 'How many lessons are there in total?', a: 'The full curriculum includes 5 levels (Beginner to Expert) with 5 modules each, totaling approximately 300 lessons. New content is added regularly, expanding the curriculum over time.' }
        ]
      },
      {
        id: 'progression',
        title: 'Progression & Support',
        questions: [
          { q: 'What if I get stuck on a lesson?', a: 'Each lesson includes practice exercises, a mini piano, and related resources. You can replay lessons as many times as needed. The quizzes give detailed explanations to help you understand what you missed.' },
          { q: 'Can I skip levels if I already have experience?', a: 'Yes! Take the placement assessment to find your starting point. It evaluates your reading, rhythm, and playing skills — no need to start from Beginner if you\'re already comfortable with basics.' },
          { q: 'How long does it take to complete a level?', a: 'Most students complete the Beginner level in 2-4 months with 15-30 minutes of daily practice. Each subsequent level takes progressively longer as material becomes more complex.' },
          { q: 'Can I go back and review previous lessons?', a: 'Yes, all completed lessons remain fully accessible. We encourage revisiting earlier material — reviewing fundamentals with fresh eyes often reveals new insights.' },
          { q: 'Do I earn certificates or badges?', a: 'Yes. Completing modules and levels earns you achievement badges displayed on your profile. These milestones help you visualize your journey and celebrate your progress.' },
          { q: 'Is there a community or forum for students?', a: 'We are building community features. Currently, you can share achievements on social media. Follow us for updates on upcoming community forums and student interaction features.' },
          { q: 'Can I access lessons on multiple devices?', a: 'Yes. Your lesson progress syncs across all devices. Start a lesson on your computer and continue on your tablet — everything is saved to your account automatically.' },
          { q: 'What happens after I complete all levels?', a: 'Advanced and Expert levels include increasingly challenging repertoire, jazz harmony, improvisation, and style-specific techniques. Beyond the curriculum, use our sheet music library and games for continued growth.' }
        ]
      }
    ]
  },
  {
    id: 'digital-tools-technology',
    title: 'Digital Tools & Technology',
    description: 'iPads for musicians, apps, software, and digital setup tips.',
    subcategories: [
      {
        id: 'ipad-for-musicians',
        title: 'iPad for Musicians',
        questions: [
          { q: 'Is 128GB storage enough for a musician\'s iPad?', a: 'For PDF sheet music, yes — 128GB holds tens of thousands of scores. For Logic Pro, audio samples, or large backups, 512GB is the recommended baseline. 1TB/2TB models also include 16GB RAM for heavier apps.' },
          { q: 'Will the OLED screen suffer from burn-in with sheet music displayed?', a: 'Apple\'s Tandem OLED uses pixel-shifting and thermal management to mitigate burn-in. The risk is minimal for typical rehearsal sessions. Set auto-lock during intermissions as a precaution.' },
          { q: 'Can I use the iPad Air 13-inch for outdoor gigs?', a: 'The Air maxes out at 600 nits vs the Pro\'s 1000-1600 nits. For direct sunlight, the Pro is significantly better. With a matte screen protector, the Air works in shaded outdoor settings.' },
          { q: 'Which iPad stand is best for touring musicians?', a: 'The KraftGeek Capsule Stand balances portability and height — collapses for carry-on bags. For heavy-duty touring, the Hercules DG305B clamped to a mic stand is the gold standard.' },
          { q: 'Does the Apple Pencil Pro work with older iPads?', a: 'No. The Apple Pencil Pro is compatible only with M4 iPad Pro and M2 iPad Air or newer. Older iPads use the Apple Pencil 2nd Gen.' },
          { q: 'Can I power my audio interface directly from the iPad?', a: 'Yes, USB-C iPads provide bus power for interfaces like the Focusrite Scarlett 2i2. However, this drains the battery quickly. Use a powered USB-C hub for live performance.' },
          { q: 'What are the best iPad apps for sheet music?', a: 'forScore is the industry standard for managing and annotating PDF sheet music. MusicNotes offers a large store with interactive playback. Both support Apple Pencil for marking up scores during practice.' },
          { q: 'Can I use my iPad as a MIDI controller?', a: 'Yes. Apps like TouchOSC and Lemur turn your iPad into a customizable MIDI controller. You can trigger sounds, control DAW parameters, and create custom performance interfaces.' }
        ]
      },
      {
        id: 'apps-software',
        title: 'Apps & Software',
        questions: [
          { q: 'What are the best piano learning apps to complement PianoMode?', a: 'A metronome app (Pro Metronome), forScore for sheet music management, and GarageBand for recording. PianoMode itself covers lessons, games, sheet music, and a virtual studio — a comprehensive toolkit.' },
          { q: 'Can I use PianoMode alongside other learning platforms?', a: 'Absolutely. PianoMode is designed to complement any learning approach. Use our sheet music for repertoire, games for skill-building, and lessons for structure — alongside your teacher or other apps.' },
          { q: 'Do I need any special software to use PianoMode?', a: 'No. PianoMode runs entirely in your web browser — no downloads or plugins. Chrome or Edge recommended for MIDI support. Safari and Firefox work great for all other features.' },
          { q: 'What is MIDI and do I need it?', a: 'MIDI (Musical Instrument Digital Interface) is a protocol that lets instruments communicate with computers. If you connect a MIDI keyboard to your computer, you can use it with the Virtual Piano Studio, games, and recording software.' },
          { q: 'Are there free alternatives to expensive music notation software?', a: 'Yes. MuseScore is a powerful free notation program. Flat.io offers browser-based notation. For simple lead sheets, even Google Docs with a music font can work. PianoMode provides pre-formatted PDF scores.' },
          { q: 'What is a DAW and do pianists need one?', a: 'A DAW (Digital Audio Workstation) is software for recording, editing, and producing music. PianoMode\'s Virtual Piano Studio includes a basic DAW. For more advanced production, GarageBand (free), Reaper (affordable), or Logic Pro are excellent choices.' },
          { q: 'Can I record my acoustic piano digitally?', a: 'Yes. Place a condenser microphone near the open lid (grand) or behind the back panel (upright), connect it to a USB audio interface, and record into any DAW. Even a smartphone with a good microphone can capture decent recordings.' },
          { q: 'How do I connect a digital piano to my computer?', a: 'Most modern digital pianos have a USB-B port. Connect it to your computer with a USB cable. The piano will appear as both a MIDI device and (on some models) an audio interface. No drivers needed on Mac; Windows may need the manufacturer\'s driver.' }
        ]
      }
    ]
  }
];

/* ═══════════════════════════════════════════
   HELPER FUNCTIONS
   ═══════════════════════════════════════════ */
function escapeHtml(s){var d=document.createElement('div');d.textContent=s;return d.innerHTML}

function highlightText(text, query) {
  if (!query) return escapeHtml(text);
  var safe = escapeHtml(text);
  var words = query.trim().split(/\s+/).filter(function(w){return w.length>1});
  if (!words.length) return safe;
  words.forEach(function(w){
    var escaped = w.replace(/[.*+?^${}()|[\]\\]/g,'\\$&');
    var re = new RegExp('('+escaped+')','gi');
    safe = safe.replace(re,'<mark class="pm-faq-highlight">$1</mark>');
  });
  return safe;
}

function debounce(fn,ms){var t;return function(){var a=arguments,c=this;clearTimeout(t);t=setTimeout(function(){fn.apply(c,a)},ms)}}

function scrollToElement(el,offset){if(!el)return;var y=el.getBoundingClientRect().top+window.pageYOffset-(offset||100);window.scrollTo({top:y,behavior:'smooth'})}

function getTotalQuestions(){var c=0;FAQ_DATA.forEach(function(cat){cat.subcategories.forEach(function(sub){c+=sub.questions.length})});return c}

/** Multi-keyword AND search: all words must match in Q or A */
function matchesSearch(q, a, query) {
  if (!query) return true;
  var words = query.toLowerCase().trim().split(/\s+/).filter(function(w){return w.length>1});
  if (!words.length) return true;
  var combined = (q + ' ' + a).toLowerCase();
  return words.every(function(w){ return combined.indexOf(w) !== -1 });
}

function getIcon(catId) {
  return FAQ_ICONS[catId] || '';
}

/* ═══════════════════════════════════════════
   RENDER FUNCTIONS
   ═══════════════════════════════════════════ */
function renderCategoryCards() {
  var container = document.getElementById('pm-faq-categories');
  if (!container) return;
  var html = '';
  FAQ_DATA.forEach(function(cat) {
    var qCount = 0;
    var subsHtml = '';
    cat.subcategories.forEach(function(sub) {
      qCount += sub.questions.length;
      subsHtml += '<span class="pm-faq-cat-card__sub-pill" data-sub="'+sub.id+'">'+escapeHtml(sub.title)+'</span>';
    });
    html += '<div class="pm-faq-cat-card" data-cat="'+cat.id+'" role="button" tabindex="0" aria-label="'+escapeHtml(cat.title)+' - '+qCount+' questions">'+
      '<span class="pm-faq-cat-card__count">'+qCount+' Q</span>'+
      '<div class="pm-faq-cat-card__header">'+
        '<div class="pm-faq-cat-card__icon">'+getIcon(cat.id)+'</div>'+
        '<h3 class="pm-faq-cat-card__title">'+escapeHtml(cat.title)+'</h3>'+
      '</div>'+
      '<p class="pm-faq-cat-card__desc">'+escapeHtml(cat.description)+'</p>'+
      '<div class="pm-faq-cat-card__subs">'+subsHtml+'</div>'+
    '</div>';
  });
  container.innerHTML = html;
}

function renderTags() {
  var container = document.getElementById('pm-faq-tags');
  if (!container) return;
  var total = getTotalQuestions();
  var html = '<button class="pm-faq-tag active" data-tag="all">All Topics <span class="pm-faq-tag-count">'+total+'</span></button>';
  FAQ_DATA.forEach(function(cat) {
    cat.subcategories.forEach(function(sub) {
      html += '<button class="pm-faq-tag" data-tag="'+sub.id+'">'+escapeHtml(sub.title)+' <span class="pm-faq-tag-count">'+sub.questions.length+'</span></button>';
    });
  });
  container.innerHTML = html;
}

function renderFAQContent(query, activeTag) {
  var container = document.getElementById('pm-faq-content');
  if (!container) return;
  var html = '';
  var totalVisible = 0;
  var q = (query || '').trim();
  var tag = activeTag || 'all';

  FAQ_DATA.forEach(function(cat) {
    cat.subcategories.forEach(function(sub) {
      if (tag !== 'all' && sub.id !== tag) return;
      var questionsHtml = '';
      var visibleInSub = 0;

      sub.questions.forEach(function(item, idx) {
        if (!matchesSearch(item.q, item.a, q)) return;
        visibleInSub++;
        totalVisible++;
        var qText = q ? highlightText(item.q, q) : escapeHtml(item.q);
        var aText = q ? highlightText(item.a, q) : escapeHtml(item.a);
        var itemId = sub.id + '-q' + idx;

        questionsHtml += '<div class="pm-faq-item" data-id="'+itemId+'">'+
          '<button class="pm-faq-item__question" aria-expanded="false" aria-controls="'+itemId+'-a" id="'+itemId+'-btn">'+
            '<span>'+qText+'</span>'+
            '<svg class="pm-faq-item__chevron" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9l6 6 6-6"/></svg>'+
          '</button>'+
          '<div class="pm-faq-item__answer" id="'+itemId+'-a" role="region" aria-labelledby="'+itemId+'-btn">'+
            '<div class="pm-faq-item__answer-inner"><p>'+aText+'</p></div>'+
          '</div>'+
        '</div>';
      });

      if (visibleInSub > 0) {
        html += '<div class="pm-faq-subcat" id="sub-'+sub.id+'" data-subcat="'+sub.id+'">'+
          '<div class="pm-faq-subcat__header">'+
            '<span class="pm-faq-subcat__icon">'+getIcon(cat.id)+'</span>'+
            '<h3 class="pm-faq-subcat__title">'+escapeHtml(sub.title)+'</h3>'+
            '<span class="pm-faq-subcat__count">'+visibleInSub+'</span>'+
          '</div>'+questionsHtml+'</div>';
      }
    });
  });

  if (totalVisible === 0) {
    html = '<div class="pm-faq-no-results">'+
      '<div class="pm-faq-no-results__icon">&#128269;</div>'+
      '<p class="pm-faq-no-results__title">No results found</p>'+
      '<p class="pm-faq-no-results__text">Try different keywords or browse all topics using the tags above.</p>'+
    '</div>';
  }

  container.innerHTML = html;

  var resultsEl = document.getElementById('pm-faq-search-results');
  if (resultsEl) {
    if (q) {
      resultsEl.style.display = 'block';
      resultsEl.innerHTML = '<strong>'+totalVisible+'</strong> question'+(totalVisible!==1?'s':'')+' found for "<strong>'+escapeHtml(q)+'</strong>"';
    } else {
      resultsEl.style.display = 'none';
    }
  }

  if (q && totalVisible > 0 && totalVisible <= 5) {
    var first = container.querySelector('.pm-faq-item');
    if (first) toggleAccordionItem(first);
  }
}

/* ═══════════════════════════════════════════
   ACCORDION TOGGLE
   ═══════════════════════════════════════════ */
function toggleAccordionItem(item) {
  if (!item) return;
  var isActive = item.classList.contains('active');
  var parent = item.closest('.pm-faq-subcat');
  if (parent) {
    parent.querySelectorAll('.pm-faq-item.active').forEach(function(el) {
      el.classList.remove('active');
      var b = el.querySelector('.pm-faq-item__question');
      if (b) b.setAttribute('aria-expanded','false');
    });
  }
  if (!isActive) {
    item.classList.add('active');
    var b = item.querySelector('.pm-faq-item__question');
    if (b) b.setAttribute('aria-expanded','true');
  }
}

/* ═══════════════════════════════════════════
   INIT FUNCTIONS
   ═══════════════════════════════════════════ */
var currentTag = 'all';
var currentQuery = '';

function initSearch() {
  var input = document.getElementById('pm-faq-search');
  var clearBtn = document.getElementById('pm-faq-search-clear');
  if (!input) return;
  var handler = debounce(function() {
    currentQuery = input.value;
    if (clearBtn) clearBtn.style.display = currentQuery ? 'flex' : 'none';
    renderFAQContent(currentQuery, currentTag);
  }, 250);
  input.addEventListener('input', handler);
  if (clearBtn) {
    clearBtn.addEventListener('click', function() {
      input.value = '';
      currentQuery = '';
      clearBtn.style.display = 'none';
      renderFAQContent('', currentTag);
      input.focus();
    });
  }
}

function initTags() {
  var container = document.getElementById('pm-faq-tags');
  if (!container) return;
  container.addEventListener('click', function(e) {
    var tag = e.target.closest('.pm-faq-tag');
    if (!tag) return;
    container.querySelectorAll('.pm-faq-tag').forEach(function(t){t.classList.remove('active')});
    tag.classList.add('active');
    currentTag = tag.getAttribute('data-tag');
    renderFAQContent(currentQuery, currentTag);
    var content = document.getElementById('pm-faq-content');
    if (content && currentTag !== 'all') scrollToElement(content, 120);
  });
}

function initCategoryCards() {
  var container = document.getElementById('pm-faq-categories');
  if (!container) return;
  container.addEventListener('click', function(e) {
    var pill = e.target.closest('.pm-faq-cat-card__sub-pill');
    if (pill) {
      var subId = pill.getAttribute('data-sub');
      if (subId) {
        currentTag = subId;
        var tags = document.getElementById('pm-faq-tags');
        if (tags) tags.querySelectorAll('.pm-faq-tag').forEach(function(t){t.classList.toggle('active',t.getAttribute('data-tag')===subId)});
        renderFAQContent(currentQuery, currentTag);
        var target = document.getElementById('sub-'+subId);
        if (target) scrollToElement(target, 100);
      }
      return;
    }
    var card = e.target.closest('.pm-faq-cat-card');
    if (card) {
      var catId = card.getAttribute('data-cat');
      var cat = FAQ_DATA.find(function(c){return c.id===catId});
      if (cat && cat.subcategories.length) {
        currentTag = 'all';
        var tags = document.getElementById('pm-faq-tags');
        if (tags) tags.querySelectorAll('.pm-faq-tag').forEach(function(t){t.classList.toggle('active',t.getAttribute('data-tag')==='all')});
        renderFAQContent(currentQuery, 'all');
        requestAnimationFrame(function(){
          var target = document.getElementById('sub-'+cat.subcategories[0].id);
          if (target) scrollToElement(target, 100);
        });
      }
    }
  });
  container.addEventListener('keydown', function(e) {
    if (e.key==='Enter'||e.key===' ') {
      var card = e.target.closest('.pm-faq-cat-card');
      if (card) { e.preventDefault(); card.click(); }
    }
  });
}

function initAccordion() {
  var container = document.getElementById('pm-faq-content');
  if (!container) return;
  container.addEventListener('click', function(e) {
    var btn = e.target.closest('.pm-faq-item__question');
    if (btn) toggleAccordionItem(btn.closest('.pm-faq-item'));
  });
  container.addEventListener('keydown', function(e) {
    if (e.key==='Enter'||e.key===' ') {
      var btn = e.target.closest('.pm-faq-item__question');
      if (btn) { e.preventDefault(); toggleAccordionItem(btn.closest('.pm-faq-item')); }
    }
  });
}

function initBackToTop() {
  var btn = document.getElementById('pm-faq-top-btn');
  if (!btn) return;
  btn.addEventListener('click', function() {
    var hero = document.querySelector('.pm-faq-hero');
    if (hero) scrollToElement(hero, 0);
    else window.scrollTo({top:0,behavior:'smooth'});
  });
}

/* ═══════════════════════════════════════════
   INIT ON DOM READY
   ═══════════════════════════════════════════ */
document.addEventListener('DOMContentLoaded', function() {
  renderCategoryCards();
  renderTags();
  renderFAQContent('', 'all');
  initSearch();
  initTags();
  initCategoryCards();
  initAccordion();
  initBackToTop();
});