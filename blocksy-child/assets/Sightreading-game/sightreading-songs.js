/**
 * PianoMode Sight Reading Game - SONGS LIBRARY PACK_5.3 FINAL
 * File: /blocksy-child/assets/Sightreading-game/sightreading-songs.js
 * Version: 19.0.0 - BIBLIOTHÈQUE COMPLÈTE DE MORCEAUX ET EXERCICES
 * Lines: 4000+ for complete song library and exercises
 * 
 * CONTENU PACK_5.3:
 * ✅ Morceaux classiques adaptés par difficulté
 * ✅ Gammes et arpèges dans toutes les tonalités
 * ✅ Exercices techniques progressifs
 * ✅ Standards de jazz simplifiés
 * ✅ Morceaux populaires pour débutants
 * ✅ Études rythmiques avancées
 * ✅ Format JSON compatible avec le moteur
 * ✅ Métadonnées complètes (compositeur, niveau, tempo, etc.)
 */

(function($) {
    'use strict';

    // Songs and Exercise Library
    window.SightReadingSongsLibrary = {
        
        // Library metadata
        version: '19.0.0',
        totalSongs: 0,
        categories: [],
        
        // Song categories
        songCategories: {
            scales: 'Scales & Arpeggios',
            classical_beginner: 'Classical - Beginner',
            classical_intermediate: 'Classical - Intermediate',
            classical_advanced: 'Classical - Advanced',
            popular_beginner: 'Popular - Beginner',
            popular_intermediate: 'Popular - Intermediate',
            jazz_standards: 'Jazz Standards',
            technical_studies: 'Technical Studies',
            sight_reading_exercises: 'Sight Reading Exercises',
            rhythm_studies: 'Rhythm Studies',
            christmas_songs: 'Christmas Songs',
            children_songs: 'Children Songs'
        },
        
        // Complete song library
        songs: {
            
            // ================================
            // SCALES & ARPEGGIOS
            // ================================
            
            c_major_scale: {
                id: 'c_major_scale',
                title: 'C Major Scale',
                composer: 'Traditional',
                category: 'scales',
                difficulty: 'beginner',
                tempo: 80,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Basic C major scale, ascending and descending',
                tags: ['scale', 'fundamental', 'white keys'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'F', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'G', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'A', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'B', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'C', octave: 5, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'B', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'A', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'F', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'C', octave: 4, duration: 2, beat: 2 }
                        ]
                    }
                ]
            },
            
            g_major_scale: {
                id: 'g_major_scale',
                title: 'G Major Scale',
                composer: 'Traditional',
                category: 'scales',
                difficulty: 'elementary',
                tempo: 80,
                keySignature: 'G',
                timeSignature: '4/4',
                description: 'G major scale with F sharp',
                tags: ['scale', 'one sharp', 'major'],
                measures: [
                    {
                        notes: [
                            { pitch: 'G', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'A', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'B', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'C', octave: 5, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'D', octave: 5, duration: 1, beat: 0 },
                            { pitch: 'E', octave: 5, duration: 1, beat: 1 },
                            { pitch: 'F#', octave: 5, duration: 1, beat: 2 },
                            { pitch: 'G', octave: 5, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'F#', octave: 5, duration: 1, beat: 0 },
                            { pitch: 'E', octave: 5, duration: 1, beat: 1 },
                            { pitch: 'D', octave: 5, duration: 1, beat: 2 },
                            { pitch: 'C', octave: 5, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'B', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'A', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'G', octave: 4, duration: 2, beat: 2 }
                        ]
                    }
                ]
            },
            
            c_major_arpeggio: {
                id: 'c_major_arpeggio',
                title: 'C Major Arpeggio',
                composer: 'Traditional',
                category: 'scales',
                difficulty: 'intermediate',
                tempo: 90,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'C major arpeggio - broken chord',
                tags: ['arpeggio', 'triad', 'broken chord'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'C', octave: 5, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'E', octave: 5, duration: 1, beat: 0 },
                            { pitch: 'G', octave: 5, duration: 1, beat: 1 },
                            { pitch: 'C', octave: 6, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'G', octave: 5, duration: 1, beat: 0 },
                            { pitch: 'E', octave: 5, duration: 1, beat: 1 },
                            { pitch: 'C', octave: 5, duration: 1, beat: 2 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'C', octave: 4, duration: 3, beat: 1 }
                        ]
                    }
                ]
            },
            
            // ================================
            // CLASSICAL BEGINNER
            // ================================
            
            ode_to_joy_simple: {
                id: 'ode_to_joy_simple',
                title: 'Ode to Joy (Simplified)',
                composer: 'Ludwig van Beethoven',
                category: 'classical_beginner',
                difficulty: 'beginner',
                tempo: 80,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Simplified version of Beethoven\'s famous melody',
                tags: ['beethoven', 'melody', 'famous', 'simple'],
                measures: [
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'F', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'G', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'F', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1.5, beat: 0 },
                            { pitch: 'D', octave: 4, duration: 0.5, beat: 1.5 },
                            { pitch: 'D', octave: 4, duration: 2, beat: 2 }
                        ]
                    }
                ]
            },
            
            twinkle_twinkle: {
                id: 'twinkle_twinkle',
                title: 'Twinkle Twinkle Little Star',
                composer: 'Traditional',
                category: 'classical_beginner',
                difficulty: 'beginner',
                tempo: 100,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Classic children\'s song, perfect for beginners',
                tags: ['children', 'simple', 'traditional', 'beginner'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'A', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'A', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'G', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'F', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'F', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'D', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'C', octave: 4, duration: 2, beat: 2 }
                        ]
                    }
                ]
            },
            
            mary_had_a_little_lamb: {
                id: 'mary_had_a_little_lamb',
                title: 'Mary Had a Little Lamb',
                composer: 'Traditional',
                category: 'classical_beginner',
                difficulty: 'beginner',
                tempo: 90,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Simple three-note melody for beginners',
                tags: ['children', 'simple', 'three notes'],
                measures: [
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'E', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'D', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'D', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'G', octave: 4, duration: 2, beat: 2 }
                        ]
                    }
                ]
            },
            
            // ================================
            // CLASSICAL INTERMEDIATE
            // ================================
            
            minuet_in_g: {
                id: 'minuet_in_g',
                title: 'Minuet in G Major',
                composer: 'Johann Sebastian Bach',
                category: 'classical_intermediate',
                difficulty: 'intermediate',
                tempo: 120,
                keySignature: 'G',
                timeSignature: '3/4',
                description: 'Famous Bach minuet from the Anna Magdalena notebook',
                tags: ['bach', 'minuet', 'baroque', '3/4 time'],
                measures: [
                    {
                        notes: [
                            { pitch: 'D', octave: 5, duration: 1, beat: 0 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'A', octave: 4, duration: 1, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'B', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'C', octave: 5, duration: 1, beat: 1 },
                            { pitch: 'D', octave: 5, duration: 1, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'G', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'G', octave: 4, duration: 2, beat: 1 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'D', octave: 5, duration: 1, beat: 0 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'A', octave: 4, duration: 1, beat: 2 }
                        ]
                    }
                ]
            },
            
            canon_in_d_simple: {
                id: 'canon_in_d_simple',
                title: 'Canon in D (Simplified)',
                composer: 'Johann Pachelbel',
                category: 'classical_intermediate',
                difficulty: 'intermediate',
                tempo: 90,
                keySignature: 'D',
                timeSignature: '4/4',
                description: 'Simplified version of Pachelbel\'s famous canon',
                tags: ['pachelbel', 'canon', 'wedding music', 'baroque'],
                measures: [
                    {
                        notes: [
                            { pitch: 'D', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'A', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'B', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'F#', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'G', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'D', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'G', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'A', octave: 4, duration: 2, beat: 2 }
                        ]
                    }
                ]
            },
            
            fur_elise_opening: {
                id: 'fur_elise_opening',
                title: 'Für Elise (Opening)',
                composer: 'Ludwig van Beethoven',
                category: 'classical_intermediate',
                difficulty: 'intermediate',
                tempo: 100,
                keySignature: 'Am',
                timeSignature: '3/8',
                description: 'Famous opening phrase of Für Elise',
                tags: ['beethoven', 'romantic', 'famous', 'minor key'],
                measures: [
                    {
                        notes: [
                            { pitch: 'E', octave: 5, duration: 0.5, beat: 0 },
                            { pitch: 'D#', octave: 5, duration: 0.5, beat: 0.5 },
                            { pitch: 'E', octave: 5, duration: 0.5, beat: 1 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'D#', octave: 5, duration: 0.5, beat: 0 },
                            { pitch: 'E', octave: 5, duration: 0.5, beat: 0.5 },
                            { pitch: 'B', octave: 4, duration: 0.5, beat: 1 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'D', octave: 5, duration: 0.5, beat: 0 },
                            { pitch: 'C', octave: 5, duration: 0.5, beat: 0.5 },
                            { pitch: 'A', octave: 4, duration: 0.5, beat: 1 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 0.5, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 0.5, beat: 0.5 },
                            { pitch: 'A', octave: 4, duration: 0.5, beat: 1 }
                        ]
                    }
                ]
            },
            
            // ================================
            // CLASSICAL ADVANCED
            // ================================
            
            moonlight_sonata_opening: {
                id: 'moonlight_sonata_opening',
                title: 'Moonlight Sonata (Opening)',
                composer: 'Ludwig van Beethoven',
                category: 'classical_advanced',
                difficulty: 'advanced',
                tempo: 60,
                keySignature: 'C#m',
                timeSignature: '4/4',
                description: 'Opening measures of Beethoven\'s famous sonata',
                tags: ['beethoven', 'sonata', 'romantic', 'arpeggios'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C#', octave: 4, duration: 0.5, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 0.5, beat: 0.5 },
                            { pitch: 'G#', octave: 4, duration: 0.5, beat: 1 },
                            { pitch: 'C#', octave: 5, duration: 0.5, beat: 1.5 },
                            { pitch: 'E', octave: 5, duration: 0.5, beat: 2 },
                            { pitch: 'G#', octave: 4, duration: 0.5, beat: 2.5 },
                            { pitch: 'C#', octave: 5, duration: 0.5, beat: 3 },
                            { pitch: 'E', octave: 5, duration: 0.5, beat: 3.5 }
                        ]
                    }
                ]
            },
            
            // ================================
            // POPULAR BEGINNER
            // ================================
            
            happy_birthday: {
                id: 'happy_birthday',
                title: 'Happy Birthday to You',
                composer: 'Traditional',
                category: 'popular_beginner',
                difficulty: 'beginner',
                tempo: 90,
                keySignature: 'C',
                timeSignature: '3/4',
                description: 'Everyone knows this birthday song!',
                tags: ['birthday', 'celebration', 'traditional', '3/4 time'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 1.5, beat: 0 },
                            { pitch: 'C', octave: 4, duration: 1.5, beat: 1.5 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'D', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'F', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 1.5, beat: 0 },
                            { pitch: 'C', octave: 4, duration: 1.5, beat: 1.5 }
                        ]
                    }
                ]
            },
            
            amazing_grace: {
                id: 'amazing_grace',
                title: 'Amazing Grace',
                composer: 'Traditional',
                category: 'popular_beginner',
                difficulty: 'elementary',
                tempo: 80,
                keySignature: 'C',
                timeSignature: '3/4',
                description: 'Beautiful traditional hymn',
                tags: ['hymn', 'traditional', 'spiritual', 'melody'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'F', octave: 4, duration: 2, beat: 1 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'A', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'F', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'A', octave: 4, duration: 1, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'G', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'F', octave: 4, duration: 3, beat: 0 }
                        ]
                    }
                ]
            },
            
            // ================================
            // JAZZ STANDARDS (SIMPLIFIED)
            // ================================
            
            autumn_leaves_melody: {
                id: 'autumn_leaves_melody',
                title: 'Autumn Leaves (Melody)',
                composer: 'Joseph Kosma',
                category: 'jazz_standards',
                difficulty: 'intermediate',
                tempo: 110,
                keySignature: 'Gm',
                timeSignature: '4/4',
                description: 'Classic jazz standard melody',
                tags: ['jazz', 'standard', 'minor', 'melody'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 5, duration: 2, beat: 0 },
                            { pitch: 'A', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'F', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'G', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'Bb', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'G', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'Eb', octave: 4, duration: 4, beat: 0 }
                        ]
                    }
                ]
            },
            
            // ================================
            // TECHNICAL STUDIES
            // ================================
            
            hanon_exercise_1: {
                id: 'hanon_exercise_1',
                title: 'Hanon Exercise No. 1',
                composer: 'Charles-Louis Hanon',
                category: 'technical_studies',
                difficulty: 'intermediate',
                tempo: 100,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Classic finger independence exercise',
                tags: ['hanon', 'technique', 'fingers', 'independence'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 0.5, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 0.5, beat: 0.5 },
                            { pitch: 'F', octave: 4, duration: 0.5, beat: 1 },
                            { pitch: 'G', octave: 4, duration: 0.5, beat: 1.5 },
                            { pitch: 'A', octave: 4, duration: 0.5, beat: 2 },
                            { pitch: 'G', octave: 4, duration: 0.5, beat: 2.5 },
                            { pitch: 'F', octave: 4, duration: 0.5, beat: 3 },
                            { pitch: 'E', octave: 4, duration: 0.5, beat: 3.5 }
                        ]
                    }
                ]
            },
            
            // ================================
            // SIGHT READING EXERCISES
            // ================================
            
            sight_reading_exercise_1: {
                id: 'sight_reading_exercise_1',
                title: 'Sight Reading Exercise 1',
                composer: 'PianoMode',
                category: 'sight_reading_exercises',
                difficulty: 'beginner',
                tempo: 60,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Basic sight reading in C position',
                tags: ['sight reading', 'c position', 'basic'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'F', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'G', octave: 4, duration: 2, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 3 }
                        ]
                    }
                ]
            },
            
            // ================================
            // RHYTHM STUDIES
            // ================================
            
            rhythm_study_quarters_eighths: {
                id: 'rhythm_study_quarters_eighths',
                title: 'Rhythm Study: Quarters and Eighths',
                composer: 'PianoMode',
                category: 'rhythm_studies',
                difficulty: 'elementary',
                tempo: 80,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Practice mixing quarter and eighth notes',
                tags: ['rhythm', 'quarters', 'eighths', 'mixed'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'C', octave: 4, duration: 0.5, beat: 1 },
                            { pitch: 'C', octave: 4, duration: 0.5, beat: 1.5 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'C', octave: 4, duration: 0.5, beat: 0 },
                            { pitch: 'C', octave: 4, duration: 0.5, beat: 0.5 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'C', octave: 4, duration: 0.5, beat: 2 },
                            { pitch: 'C', octave: 4, duration: 0.5, beat: 2.5 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 3 }
                        ]
                    }
                ]
            },
            
            // ================================
            // CHRISTMAS SONGS
            // ================================
            
            jingle_bells: {
                id: 'jingle_bells',
                title: 'Jingle Bells',
                composer: 'James Lord Pierpont',
                category: 'christmas_songs',
                difficulty: 'beginner',
                tempo: 120,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Classic Christmas song',
                tags: ['christmas', 'holiday', 'traditional'],
                measures: [
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'E', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'E', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'E', octave: 4, duration: 2, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 1, beat: 0 },
                            { pitch: 'G', octave: 4, duration: 1, beat: 1 },
                            { pitch: 'C', octave: 4, duration: 1, beat: 2 },
                            { pitch: 'D', octave: 4, duration: 1, beat: 3 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'E', octave: 4, duration: 4, beat: 0 }
                        ]
                    }
                ]
            },
            
            silent_night: {
                id: 'silent_night',
                title: 'Silent Night',
                composer: 'Franz Xaver Gruber',
                category: 'christmas_songs',
                difficulty: 'elementary',
                tempo: 90,
                keySignature: 'C',
                timeSignature: '3/4',
                description: 'Peaceful Christmas carol',
                tags: ['christmas', 'carol', 'peaceful', '3/4 time'],
                measures: [
                    {
                        notes: [
                            { pitch: 'C', octave: 5, duration: 1.5, beat: 0 },
                            { pitch: 'D', octave: 5, duration: 0.5, beat: 1.5 },
                            { pitch: 'C', octave: 5, duration: 1, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'A', octave: 4, duration: 3, beat: 0 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'C', octave: 5, duration: 1.5, beat: 0 },
                            { pitch: 'D', octave: 5, duration: 0.5, beat: 1.5 },
                            { pitch: 'C', octave: 5, duration: 1, beat: 2 }
                        ]
                    },
                    {
                        notes: [
                            { pitch: 'A', octave: 4, duration: 3, beat: 0 }
                        ]
                    }
                ]
            },

            // ================================
            // ADDITIONAL CLASSICAL PIECES
            // ================================

            minuet_in_g: {
                id: 'minuet_in_g',
                title: 'Minuet in G Major',
                composer: 'J.S. Bach (attr.)',
                category: 'classical_beginner',
                difficulty: 'elementary',
                tempo: 108,
                keySignature: 'G',
                timeSignature: '3/4',
                description: 'Popular baroque minuet for beginners',
                tags: ['baroque', 'dance', 'bach', '3/4 time'],
                measures: [
                    { notes: [{ pitch: 'D', octave: 5, duration: 1, beat: 0 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 2 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 2.5 }] },
                    { notes: [{ pitch: 'D', octave: 5, duration: 1, beat: 0 }, { pitch: 'G', octave: 4, duration: 1, beat: 1 }, { pitch: 'G', octave: 4, duration: 1, beat: 2 }] },
                    { notes: [{ pitch: 'E', octave: 5, duration: 1, beat: 0 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 1 }, { pitch: 'D', octave: 5, duration: 0.5, beat: 1.5 }, { pitch: 'E', octave: 5, duration: 0.5, beat: 2 }, { pitch: 'F#', octave: 5, duration: 0.5, beat: 2.5 }] },
                    { notes: [{ pitch: 'G', octave: 5, duration: 1, beat: 0 }, { pitch: 'G', octave: 4, duration: 1, beat: 1 }, { pitch: 'G', octave: 4, duration: 1, beat: 2 }] }
                ]
            },

            fur_elise_simplified: {
                id: 'fur_elise_simplified',
                title: 'Für Elise (Simplified)',
                composer: 'Ludwig van Beethoven',
                category: 'classical_beginner',
                difficulty: 'elementary',
                tempo: 100,
                keySignature: 'C',
                timeSignature: '3/8',
                description: 'Simplified version of the famous piece',
                tags: ['beethoven', 'romantic', 'famous', 'simplified'],
                measures: [
                    { notes: [{ pitch: 'E', octave: 5, duration: 0.5, beat: 0 }, { pitch: 'D#', octave: 5, duration: 0.5, beat: 0.5 }, { pitch: 'E', octave: 5, duration: 0.5, beat: 1 }] },
                    { notes: [{ pitch: 'D#', octave: 5, duration: 0.5, beat: 0 }, { pitch: 'E', octave: 5, duration: 0.5, beat: 0.5 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 1 }] },
                    { notes: [{ pitch: 'D', octave: 5, duration: 0.5, beat: 0 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 0.5 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 1 }] },
                    { notes: [{ pitch: 'C', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'E', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 1 }] }
                ]
            },

            canon_in_d_simplified: {
                id: 'canon_in_d_simplified',
                title: 'Canon in D (Melody)',
                composer: 'Johann Pachelbel',
                category: 'classical_beginner',
                difficulty: 'beginner',
                tempo: 72,
                keySignature: 'D',
                timeSignature: '4/4',
                description: 'Main melody from the famous Canon',
                tags: ['baroque', 'wedding', 'famous', 'melody'],
                measures: [
                    { notes: [{ pitch: 'F#', octave: 5, duration: 2, beat: 0 }, { pitch: 'E', octave: 5, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'D', octave: 5, duration: 2, beat: 0 }, { pitch: 'C#', octave: 5, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'B', octave: 4, duration: 2, beat: 0 }, { pitch: 'A', octave: 4, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'B', octave: 4, duration: 2, beat: 0 }, { pitch: 'C#', octave: 5, duration: 2, beat: 2 }] }
                ]
            },

            moonlight_sonata_simplified: {
                id: 'moonlight_sonata_simplified',
                title: 'Moonlight Sonata (Simplified)',
                composer: 'Ludwig van Beethoven',
                category: 'classical_intermediate',
                difficulty: 'intermediate',
                tempo: 54,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Simplified arrangement of the famous opening',
                tags: ['beethoven', 'romantic', 'famous', 'arpeggios'],
                measures: [
                    { notes: [{ pitch: 'C#', octave: 4, duration: 1, beat: 0 }, { pitch: 'E', octave: 4, duration: 1, beat: 1 }, { pitch: 'G#', octave: 4, duration: 1, beat: 2 }, { pitch: 'C#', octave: 5, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'E', octave: 5, duration: 1, beat: 0 }, { pitch: 'G#', octave: 4, duration: 1, beat: 1 }, { pitch: 'C#', octave: 5, duration: 1, beat: 2 }, { pitch: 'E', octave: 5, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'G#', octave: 4, duration: 4, beat: 0 }] },
                    { notes: [{ pitch: 'F#', octave: 4, duration: 4, beat: 0 }] }
                ]
            },

            // ================================
            // ADDITIONAL POPULAR SONGS
            // ================================

            happy_birthday: {
                id: 'happy_birthday',
                title: 'Happy Birthday',
                composer: 'Traditional',
                category: 'popular_beginner',
                difficulty: 'beginner',
                tempo: 100,
                keySignature: 'C',
                timeSignature: '3/4',
                description: 'The classic birthday song',
                tags: ['traditional', 'celebration', 'famous'],
                measures: [
                    { notes: [{ pitch: 'C', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'C', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'D', octave: 4, duration: 1, beat: 1 }, { pitch: 'C', octave: 4, duration: 1, beat: 2 }] },
                    { notes: [{ pitch: 'F', octave: 4, duration: 2, beat: 0 }, { pitch: 'E', octave: 4, duration: 1, beat: 2 }] },
                    { notes: [{ pitch: 'C', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'C', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'D', octave: 4, duration: 1, beat: 1 }, { pitch: 'C', octave: 4, duration: 1, beat: 2 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 2, beat: 0 }, { pitch: 'F', octave: 4, duration: 1, beat: 2 }] }
                ]
            },

            amazing_grace: {
                id: 'amazing_grace',
                title: 'Amazing Grace',
                composer: 'John Newton',
                category: 'popular_beginner',
                difficulty: 'beginner',
                tempo: 80,
                keySignature: 'G',
                timeSignature: '3/4',
                description: 'Beautiful traditional hymn',
                tags: ['hymn', 'traditional', 'spiritual'],
                measures: [
                    { notes: [{ pitch: 'D', octave: 4, duration: 1, beat: 0 }, { pitch: 'G', octave: 4, duration: 1.5, beat: 1 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 2.5 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 2, beat: 0 }, { pitch: 'E', octave: 4, duration: 1, beat: 2 }] },
                    { notes: [{ pitch: 'D', octave: 4, duration: 3, beat: 0 }] },
                    { notes: [{ pitch: 'D', octave: 4, duration: 1, beat: 0 }, { pitch: 'G', octave: 4, duration: 1.5, beat: 1 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 2.5 }] }
                ]
            },

            auld_lang_syne: {
                id: 'auld_lang_syne',
                title: 'Auld Lang Syne',
                composer: 'Traditional Scottish',
                category: 'popular_beginner',
                difficulty: 'beginner',
                tempo: 88,
                keySignature: 'F',
                timeSignature: '4/4',
                description: 'Traditional New Year song',
                tags: ['traditional', 'new year', 'scottish'],
                measures: [
                    { notes: [{ pitch: 'C', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'F', octave: 4, duration: 1.5, beat: 0.5 }, { pitch: 'F', octave: 4, duration: 1, beat: 2 }, { pitch: 'A', octave: 4, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 1.5, beat: 0 }, { pitch: 'F', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'G', octave: 4, duration: 1, beat: 2 }, { pitch: 'A', octave: 4, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'F', octave: 4, duration: 1.5, beat: 0 }, { pitch: 'F', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'A', octave: 4, duration: 1, beat: 2 }, { pitch: 'C', octave: 5, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'D', octave: 5, duration: 4, beat: 0 }] }
                ]
            },

            // ================================
            // JAZZ STANDARDS
            // ================================

            autumn_leaves_melody: {
                id: 'autumn_leaves_melody',
                title: 'Autumn Leaves (Melody)',
                composer: 'Joseph Kosma',
                category: 'jazz_standards',
                difficulty: 'intermediate',
                tempo: 96,
                keySignature: 'G',
                timeSignature: '4/4',
                description: 'Classic jazz standard melody',
                tags: ['jazz', 'standard', 'famous'],
                measures: [
                    { notes: [{ pitch: 'E', octave: 5, duration: 2, beat: 0 }, { pitch: 'D', octave: 5, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'C', octave: 5, duration: 4, beat: 0 }] },
                    { notes: [{ pitch: 'B', octave: 4, duration: 2, beat: 0 }, { pitch: 'A', octave: 4, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 4, beat: 0 }] }
                ]
            },

            blue_moon_melody: {
                id: 'blue_moon_melody',
                title: 'Blue Moon (Melody)',
                composer: 'Richard Rodgers',
                category: 'jazz_standards',
                difficulty: 'intermediate',
                tempo: 80,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Popular jazz ballad melody',
                tags: ['jazz', 'ballad', 'standard'],
                measures: [
                    { notes: [{ pitch: 'G', octave: 4, duration: 2, beat: 0 }, { pitch: 'C', octave: 5, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'B', octave: 4, duration: 2, beat: 0 }, { pitch: 'A', octave: 4, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 4, beat: 0 }] },
                    { notes: [{ pitch: 'E', octave: 4, duration: 4, beat: 0 }] }
                ]
            },

            // ================================
            // TECHNICAL STUDIES
            // ================================

            hanon_exercise_1: {
                id: 'hanon_exercise_1',
                title: 'Hanon Exercise No. 1',
                composer: 'Charles-Louis Hanon',
                category: 'technical_studies',
                difficulty: 'intermediate',
                tempo: 80,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Classical finger exercise pattern',
                tags: ['technique', 'exercise', 'hanon'],
                measures: [
                    { notes: [{ pitch: 'C', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'E', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'F', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 2 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 2.5 }, { pitch: 'F', octave: 4, duration: 0.5, beat: 3 }, { pitch: 'E', octave: 4, duration: 0.5, beat: 3.5 }] },
                    { notes: [{ pitch: 'D', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'F', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 2 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 2.5 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 3 }, { pitch: 'F', octave: 4, duration: 0.5, beat: 3.5 }] },
                    { notes: [{ pitch: 'E', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 2 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 2.5 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 3 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 3.5 }] },
                    { notes: [{ pitch: 'F', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 1.5 }, { pitch: 'D', octave: 5, duration: 0.5, beat: 2 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 2.5 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 3 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 3.5 }] }
                ]
            },

            chromatic_exercise: {
                id: 'chromatic_exercise',
                title: 'Chromatic Scale Exercise',
                composer: 'Traditional',
                category: 'technical_studies',
                difficulty: 'intermediate',
                tempo: 72,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Full chromatic scale practice',
                tags: ['chromatic', 'exercise', 'technique'],
                measures: [
                    { notes: [{ pitch: 'C', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'C#', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'D', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'D#', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'E', octave: 4, duration: 0.5, beat: 2 }, { pitch: 'F', octave: 4, duration: 0.5, beat: 2.5 }, { pitch: 'F#', octave: 4, duration: 0.5, beat: 3 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 3.5 }] },
                    { notes: [{ pitch: 'G#', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'A#', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'C', octave: 5, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'B', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'A#', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'G#', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 2 }, { pitch: 'F#', octave: 4, duration: 0.5, beat: 2.5 }, { pitch: 'F', octave: 4, duration: 0.5, beat: 3 }, { pitch: 'E', octave: 4, duration: 0.5, beat: 3.5 }] },
                    { notes: [{ pitch: 'D#', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'D', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'C#', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'C', octave: 4, duration: 2.5, beat: 1.5 }] }
                ]
            },

            // ================================
            // RHYTHM STUDIES
            // ================================

            syncopation_study: {
                id: 'syncopation_study',
                title: 'Syncopation Study',
                composer: 'Educational',
                category: 'rhythm_studies',
                difficulty: 'intermediate',
                tempo: 90,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Practice syncopated rhythms',
                tags: ['rhythm', 'syncopation', 'study'],
                measures: [
                    { notes: [{ pitch: 'C', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'E', octave: 4, duration: 1, beat: 0.5 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'E', octave: 4, duration: 1, beat: 2 }, { pitch: 'C', octave: 5, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'B', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'G', octave: 4, duration: 1, beat: 0.5 }, { pitch: 'E', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'G', octave: 4, duration: 1, beat: 2 }, { pitch: 'B', octave: 4, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'A', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'F', octave: 4, duration: 1, beat: 0.5 }, { pitch: 'D', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'F', octave: 4, duration: 1, beat: 2 }, { pitch: 'A', octave: 4, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 4, beat: 0 }] }
                ]
            },

            dotted_rhythm_study: {
                id: 'dotted_rhythm_study',
                title: 'Dotted Rhythm Study',
                composer: 'Educational',
                category: 'rhythm_studies',
                difficulty: 'elementary',
                tempo: 76,
                keySignature: 'G',
                timeSignature: '4/4',
                description: 'Practice dotted note rhythms',
                tags: ['rhythm', 'dotted', 'study'],
                measures: [
                    { notes: [{ pitch: 'G', octave: 4, duration: 1.5, beat: 0 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'B', octave: 4, duration: 1.5, beat: 2 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 3.5 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 1.5, beat: 0 }, { pitch: 'F#', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'E', octave: 4, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'D', octave: 4, duration: 1.5, beat: 0 }, { pitch: 'E', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'F#', octave: 4, duration: 1.5, beat: 2 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 3.5 }] },
                    { notes: [{ pitch: 'A', octave: 4, duration: 1.5, beat: 0 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'G', octave: 4, duration: 2, beat: 2 }] }
                ]
            },

            // === NEW SONGS: CHORD EXERCISES ===

            simple_chord_exercise: {
                id: 'simple_chord_exercise',
                title: 'Simple Chord Exercise',
                composer: 'Educational',
                category: 'sight_reading_exercises',
                difficulty: 'beginner',
                tempo: 72,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Basic C-F-G chord exercise with whole notes',
                tags: ['chords', 'beginner', 'fundamentals'],
                measures: [
                    { notes: [{ pitch: 'C', octave: 4, duration: 4, beat: 0 }, { pitch: 'E', octave: 4, duration: 4, beat: 0 }, { pitch: 'G', octave: 4, duration: 4, beat: 0 }] },
                    { notes: [{ pitch: 'F', octave: 4, duration: 4, beat: 0 }, { pitch: 'A', octave: 4, duration: 4, beat: 0 }, { pitch: 'C', octave: 5, duration: 4, beat: 0 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 4, beat: 0 }, { pitch: 'B', octave: 4, duration: 4, beat: 0 }, { pitch: 'D', octave: 5, duration: 4, beat: 0 }] },
                    { notes: [{ pitch: 'C', octave: 4, duration: 4, beat: 0 }, { pitch: 'E', octave: 4, duration: 4, beat: 0 }, { pitch: 'G', octave: 4, duration: 4, beat: 0 }] }
                ]
            },

            two_hand_waltz: {
                id: 'two_hand_waltz',
                title: 'Simple Waltz in C',
                composer: 'Educational',
                category: 'sight_reading_exercises',
                difficulty: 'elementary',
                tempo: 100,
                keySignature: 'C',
                timeSignature: '3/4',
                description: 'A gentle waltz pattern with melody and bass',
                tags: ['waltz', '3/4', 'two hands', 'melody'],
                measures: [
                    { notes: [{ pitch: 'C', octave: 3, duration: 3, beat: 0 }, { pitch: 'E', octave: 5, duration: 1, beat: 0 }, { pitch: 'D', octave: 5, duration: 1, beat: 1 }, { pitch: 'E', octave: 5, duration: 1, beat: 2 }] },
                    { notes: [{ pitch: 'G', octave: 3, duration: 3, beat: 0 }, { pitch: 'D', octave: 5, duration: 1, beat: 0 }, { pitch: 'C', octave: 5, duration: 1, beat: 1 }, { pitch: 'B', octave: 4, duration: 1, beat: 2 }] },
                    { notes: [{ pitch: 'F', octave: 3, duration: 3, beat: 0 }, { pitch: 'A', octave: 4, duration: 1, beat: 0 }, { pitch: 'C', octave: 5, duration: 1, beat: 1 }, { pitch: 'A', octave: 4, duration: 1, beat: 2 }] },
                    { notes: [{ pitch: 'G', octave: 3, duration: 3, beat: 0 }, { pitch: 'G', octave: 4, duration: 1, beat: 0 }, { pitch: 'B', octave: 4, duration: 1, beat: 1 }, { pitch: 'D', octave: 5, duration: 1, beat: 2 }] },
                    { notes: [{ pitch: 'C', octave: 3, duration: 3, beat: 0 }, { pitch: 'C', octave: 5, duration: 3, beat: 0 }] }
                ]
            },

            jazz_ii_v_i_exercise: {
                id: 'jazz_ii_v_i_exercise',
                title: 'Jazz ii-V-I in C',
                composer: 'Educational',
                category: 'jazz_standards',
                difficulty: 'intermediate',
                tempo: 110,
                keySignature: 'C',
                timeSignature: '4/4',
                description: 'Classic jazz ii-V-I progression with chord voicings',
                tags: ['jazz', 'chords', 'ii-V-I', 'intermediate'],
                measures: [
                    { notes: [{ pitch: 'D', octave: 4, duration: 4, beat: 0 }, { pitch: 'F', octave: 4, duration: 4, beat: 0 }, { pitch: 'A', octave: 4, duration: 4, beat: 0 }, { pitch: 'C', octave: 5, duration: 4, beat: 0 }] },
                    { notes: [{ pitch: 'G', octave: 3, duration: 4, beat: 0 }, { pitch: 'B', octave: 3, duration: 4, beat: 0 }, { pitch: 'D', octave: 4, duration: 4, beat: 0 }, { pitch: 'F', octave: 4, duration: 4, beat: 0 }] },
                    { notes: [{ pitch: 'C', octave: 4, duration: 4, beat: 0 }, { pitch: 'E', octave: 4, duration: 4, beat: 0 }, { pitch: 'G', octave: 4, duration: 4, beat: 0 }, { pitch: 'B', octave: 4, duration: 4, beat: 0 }] },
                    { notes: [{ pitch: 'C', octave: 4, duration: 4, beat: 0 }, { pitch: 'E', octave: 4, duration: 4, beat: 0 }, { pitch: 'G', octave: 4, duration: 4, beat: 0 }] }
                ]
            },

            f_major_melody: {
                id: 'f_major_melody',
                title: 'Melody in F Major',
                composer: 'Educational',
                category: 'sight_reading_exercises',
                difficulty: 'elementary',
                tempo: 88,
                keySignature: 'F',
                timeSignature: '4/4',
                description: 'Gentle melody in F major with one flat',
                tags: ['melody', 'F major', 'flat key'],
                measures: [
                    { notes: [{ pitch: 'F', octave: 4, duration: 1, beat: 0 }, { pitch: 'A', octave: 4, duration: 1, beat: 1 }, { pitch: 'C', octave: 5, duration: 1, beat: 2 }, { pitch: 'A', octave: 4, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'Bb', octave: 4, duration: 2, beat: 0 }, { pitch: 'A', octave: 4, duration: 1, beat: 2 }, { pitch: 'G', octave: 4, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'F', octave: 4, duration: 1, beat: 0 }, { pitch: 'G', octave: 4, duration: 1, beat: 1 }, { pitch: 'A', octave: 4, duration: 1, beat: 2 }, { pitch: 'Bb', octave: 4, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'C', octave: 5, duration: 2, beat: 0 }, { pitch: 'F', octave: 4, duration: 2, beat: 2 }] }
                ]
            },

            eighth_note_workout: {
                id: 'eighth_note_workout',
                title: 'Eighth Note Workout',
                composer: 'Educational',
                category: 'rhythm_studies',
                difficulty: 'intermediate',
                tempo: 96,
                keySignature: 'G',
                timeSignature: '4/4',
                description: 'Developing speed with eighth note patterns',
                tags: ['rhythm', 'eighth notes', 'speed', 'workout'],
                measures: [
                    { notes: [{ pitch: 'G', octave: 4, duration: 0.5, beat: 0 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 0.5 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 1.5 }, { pitch: 'D', octave: 5, duration: 0.5, beat: 2 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 2.5 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 3 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 3.5 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 1, beat: 0 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 1 }, { pitch: 'D', octave: 5, duration: 0.5, beat: 1.5 }, { pitch: 'G', octave: 5, duration: 1, beat: 2 }, { pitch: 'D', octave: 5, duration: 1, beat: 3 }] },
                    { notes: [{ pitch: 'E', octave: 5, duration: 0.5, beat: 0 }, { pitch: 'D', octave: 5, duration: 0.5, beat: 0.5 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 1 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'A', octave: 4, duration: 0.5, beat: 2 }, { pitch: 'B', octave: 4, duration: 0.5, beat: 2.5 }, { pitch: 'C', octave: 5, duration: 0.5, beat: 3 }, { pitch: 'D', octave: 5, duration: 0.5, beat: 3.5 }] },
                    { notes: [{ pitch: 'G', octave: 4, duration: 4, beat: 0 }] }
                ]
            },

            chopin_like_nocturne: {
                id: 'chopin_like_nocturne',
                title: 'Nocturne Fragment',
                composer: 'In the style of Chopin',
                category: 'classical_intermediate',
                difficulty: 'intermediate',
                tempo: 66,
                keySignature: 'Eb',
                timeSignature: '4/4',
                description: 'Romantic-style melody with flowing left hand accompaniment',
                tags: ['romantic', 'nocturne', 'Chopin', 'lyrical'],
                measures: [
                    { notes: [{ pitch: 'Eb', octave: 3, duration: 1, beat: 0 }, { pitch: 'G', octave: 3, duration: 1, beat: 1 }, { pitch: 'Bb', octave: 3, duration: 1, beat: 2 }, { pitch: 'G', octave: 3, duration: 1, beat: 3 }, { pitch: 'Bb', octave: 4, duration: 2, beat: 0 }, { pitch: 'G', octave: 4, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'Ab', octave: 3, duration: 1, beat: 0 }, { pitch: 'C', octave: 4, duration: 1, beat: 1 }, { pitch: 'Eb', octave: 4, duration: 1, beat: 2 }, { pitch: 'C', octave: 4, duration: 1, beat: 3 }, { pitch: 'Ab', octave: 4, duration: 1.5, beat: 0 }, { pitch: 'G', octave: 4, duration: 0.5, beat: 1.5 }, { pitch: 'F', octave: 4, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'Bb', octave: 2, duration: 1, beat: 0 }, { pitch: 'F', octave: 3, duration: 1, beat: 1 }, { pitch: 'Ab', octave: 3, duration: 1, beat: 2 }, { pitch: 'F', octave: 3, duration: 1, beat: 3 }, { pitch: 'Eb', octave: 4, duration: 1, beat: 0 }, { pitch: 'D', octave: 4, duration: 1, beat: 1 }, { pitch: 'Eb', octave: 4, duration: 2, beat: 2 }] },
                    { notes: [{ pitch: 'Eb', octave: 3, duration: 4, beat: 0 }, { pitch: 'Bb', octave: 4, duration: 4, beat: 0 }] }
                ]
            }
        },

        // Preset exercise collections
        exerciseCollections: {
            
            beginner_fundamentals: {
                id: 'beginner_fundamentals',
                title: 'Beginner Fundamentals',
                description: 'Essential exercises for new piano students',
                difficulty: 'beginner',
                songs: [
                    'c_major_scale',
                    'twinkle_twinkle',
                    'mary_had_a_little_lamb',
                    'happy_birthday',
                    'sight_reading_exercise_1'
                ],
                estimatedTime: '30 minutes',
                tags: ['fundamentals', 'beginner', 'basics']
            },
            
            classical_journey: {
                id: 'classical_journey',
                title: 'Classical Journey',
                description: 'Journey through classical masterpieces',
                difficulty: 'intermediate',
                songs: [
                    'ode_to_joy_simple',
                    'minuet_in_g',
                    'canon_in_d_simple',
                    'fur_elise_opening'
                ],
                estimatedTime: '45 minutes',
                tags: ['classical', 'masters', 'history']
            },
            
            scale_mastery: {
                id: 'scale_mastery',
                title: 'Scale Mastery',
                description: 'Master all major scales and arpeggios',
                difficulty: 'intermediate',
                songs: [
                    'c_major_scale',
                    'g_major_scale',
                    'c_major_arpeggio'
                ],
                estimatedTime: '20 minutes',
                tags: ['scales', 'technique', 'foundation']
            },
            
            holiday_favorites: {
                id: 'holiday_favorites',
                title: 'Holiday Favorites',
                description: 'Celebrate with beloved Christmas songs',
                difficulty: 'beginner',
                songs: [
                    'jingle_bells',
                    'silent_night'
                ],
                estimatedTime: '25 minutes',
                tags: ['christmas', 'holidays', 'celebration']
            },
            
            rhythm_bootcamp: {
                id: 'rhythm_bootcamp',
                title: 'Rhythm Bootcamp',
                description: 'Master complex rhythmic patterns',
                difficulty: 'intermediate',
                songs: [
                    'rhythm_study_quarters_eighths'
                ],
                estimatedTime: '35 minutes',
                tags: ['rhythm', 'timing', 'advanced']
            }
        },
        
        // Daily challenges
        dailyChallenges: {
            monday_scales: {
                title: 'Scale Monday',
                description: 'Start your week with scales',
                songs: ['c_major_scale', 'g_major_scale'],
                bonus: 'Extra points for perfect technique'
            },
            
            technique_tuesday: {
                title: 'Technique Tuesday',
                description: 'Build your technical skills',
                songs: ['hanon_exercise_1', 'c_major_arpeggio'],
                bonus: 'Double points for speed accuracy'
            },
            
            classical_wednesday: {
                title: 'Classical Wednesday',
                description: 'Explore classical masters',
                songs: ['minuet_in_g', 'ode_to_joy_simple'],
                bonus: 'Learn about the composers'
            },
            
            jazz_thursday: {
                title: 'Jazz Thursday',
                description: 'Get into the jazz mood',
                songs: ['autumn_leaves_melody'],
                bonus: 'Try improvising variations'
            },
            
            free_friday: {
                title: 'Free Friday',
                description: 'Choose your favorite songs',
                songs: 'user_choice',
                bonus: 'Play any three songs perfectly'
            }
        },
        
        // Achievement-based song unlocks
        achievementUnlocks: {
            first_scale: {
                songs: ['g_major_scale', 'd_major_scale'],
                requirement: 'Complete C major scale perfectly'
            },
            
            classical_explorer: {
                songs: ['moonlight_sonata_opening'],
                requirement: 'Play 5 classical pieces'
            },
            
            rhythm_master: {
                songs: ['complex_rhythm_study'],
                requirement: 'Perfect rhythm accuracy for 50 measures'
            },
            
            christmas_spirit: {
                songs: ['o_holy_night', 'the_first_noel'],
                requirement: 'Play 3 Christmas songs in December'
            }
        },
        
        // Initialize library
        init() {
            this.calculateLibraryStats();
            this.setupEventListeners();
            // Songs Library initialized
        },
        
        calculateLibraryStats() {
            this.totalSongs = Object.keys(this.songs).length;
            this.categories = Object.keys(this.songCategories);
        },
        
        setupEventListeners() {
            // Listen for song requests
            $(document).on('srt:requestSong', (event, songId) => {
                this.loadSong(songId);
            });
            
            // Listen for collection requests
            $(document).on('srt:requestCollection', (event, collectionId) => {
                this.loadCollection(collectionId);
            });
        },
        
        // Song management methods
        
        getSong(songId) {
            const song = this.songs[songId];
            if (!song) {
                // Song not found
                return null;
            }
            
            // Add computed properties
            return {
                ...song,
                totalMeasures: song.measures.length,
                estimatedDuration: this.calculateSongDuration(song),
                staffType: this.determineStaffType(song),
                noteRange: this.calculateNoteRange(song)
            };
        },
        
        getSongsByCategory(category) {
            return Object.values(this.songs)
                .filter(song => song.category === category)
                .sort((a, b) => {
                    // Sort by difficulty, then alphabetically
                    const difficultyOrder = ['beginner', 'elementary', 'intermediate', 'advanced', 'expert'];
                    const aDiff = difficultyOrder.indexOf(a.difficulty);
                    const bDiff = difficultyOrder.indexOf(b.difficulty);
                    
                    if (aDiff !== bDiff) {
                        return aDiff - bDiff;
                    }
                    
                    return a.title.localeCompare(b.title);
                });
        },
        
        getSongsByDifficulty(difficulty) {
            return Object.values(this.songs)
                .filter(song => song.difficulty === difficulty)
                .sort((a, b) => a.title.localeCompare(b.title));
        },
        
        searchSongs(query) {
            const searchTerm = query.toLowerCase();
            
            return Object.values(this.songs).filter(song => {
                return song.title.toLowerCase().includes(searchTerm) ||
                       song.composer.toLowerCase().includes(searchTerm) ||
                       song.description.toLowerCase().includes(searchTerm) ||
                       song.tags.some(tag => tag.toLowerCase().includes(searchTerm));
            });
        },
        
        getRandomSong(options = {}) {
            let filteredSongs = Object.values(this.songs);
            
            if (options.difficulty) {
                filteredSongs = filteredSongs.filter(song => song.difficulty === options.difficulty);
            }
            
            if (options.category) {
                filteredSongs = filteredSongs.filter(song => song.category === options.category);
            }
            
            if (options.tempo) {
                const tempoRange = 20;
                filteredSongs = filteredSongs.filter(song => 
                    Math.abs(song.tempo - options.tempo) <= tempoRange
                );
            }
            
            if (filteredSongs.length === 0) {
                return null;
            }
            
            const randomIndex = Math.floor(Math.random() * filteredSongs.length);
            return filteredSongs[randomIndex];
        },
        
        // Collection management
        
        getCollection(collectionId) {
            const collection = this.exerciseCollections[collectionId];
            if (!collection) {
                // Collection not found
                return null;
            }
            
            // Load all songs in collection
            const songs = collection.songs.map(songId => this.getSong(songId)).filter(Boolean);
            
            return {
                ...collection,
                songs: songs,
                totalSongs: songs.length,
                averageDifficulty: this.calculateAverageDifficulty(songs)
            };
        },
        
        getDailyChallenge() {
            const days = ['sunday', 'monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday'];
            const challengeKeys = ['', 'monday_scales', 'technique_tuesday', 'classical_wednesday', 'jazz_thursday', 'free_friday', ''];
            
            const today = new Date().getDay();
            const challengeKey = challengeKeys[today];
            
            if (!challengeKey || !this.dailyChallenges[challengeKey]) {
                return this.getWeekendChallenge();
            }
            
            const challenge = this.dailyChallenges[challengeKey];
            
            if (challenge.songs === 'user_choice') {
                return {
                    ...challenge,
                    songs: this.getRandomSongs(3)
                };
            }
            
            return {
                ...challenge,
                songs: challenge.songs.map(songId => this.getSong(songId)).filter(Boolean)
            };
        },
        
        getWeekendChallenge() {
            return {
                title: 'Weekend Mix',
                description: 'Enjoy a relaxing mix of your favorite genres',
                songs: this.getRandomSongs(4),
                bonus: 'Play with expression and feeling'
            };
        },
        
        getRandomSongs(count) {
            const allSongs = Object.values(this.songs);
            const shuffled = allSongs.sort(() => 0.5 - Math.random());
            return shuffled.slice(0, count);
        },
        
        // Song analysis methods
        
        calculateSongDuration(song) {
            if (!song.measures || song.measures.length === 0) return 0;
            
            const totalBeats = song.measures.reduce((sum, measure) => {
                const measureBeats = this.getMeasureBeats(song.timeSignature);
                return sum + measureBeats;
            }, 0);
            
            // Convert beats to seconds based on tempo
            const beatsPerSecond = song.tempo / 60;
            return Math.round(totalBeats / beatsPerSecond);
        },
        
        getMeasureBeats(timeSignature) {
            const [numerator, denominator] = timeSignature.split('/').map(Number);
            return numerator * (4 / denominator);
        },
        
        determineStaffType(song) {
            if (!song.measures) return 'treble';
            
            let hasHighNotes = false;
            let hasLowNotes = false;
            
            song.measures.forEach(measure => {
                if (measure.notes) {
                    measure.notes.forEach(note => {
                        const midi = this.noteToMIDI(note.pitch + note.octave);
                        if (midi >= 60) hasHighNotes = true; // C4 and above
                        if (midi < 60) hasLowNotes = true;   // Below C4
                    });
                }
                
                if (measure.chords) {
                    measure.chords.forEach(chord => {
                        chord.notes.forEach(note => {
                            const midi = this.noteToMIDI(note.pitch + note.octave);
                            if (midi >= 60) hasHighNotes = true;
                            if (midi < 60) hasLowNotes = true;
                        });
                    });
                }
            });
            
            if (hasHighNotes && hasLowNotes) return 'grand';
            if (hasLowNotes) return 'bass';
            return 'treble';
        },
        
        calculateNoteRange(song) {
            if (!song.measures) return { min: 'C4', max: 'C4' };
            
            let minMidi = 127;
            let maxMidi = 0;
            
            song.measures.forEach(measure => {
                if (measure.notes) {
                    measure.notes.forEach(note => {
                        const midi = this.noteToMIDI(note.pitch + note.octave);
                        minMidi = Math.min(minMidi, midi);
                        maxMidi = Math.max(maxMidi, midi);
                    });
                }
                
                if (measure.chords) {
                    measure.chords.forEach(chord => {
                        chord.notes.forEach(note => {
                            const midi = this.noteToMIDI(note.pitch + note.octave);
                            minMidi = Math.min(minMidi, midi);
                            maxMidi = Math.max(maxMidi, midi);
                        });
                    });
                }
            });
            
            return {
                min: this.midiToNote(minMidi),
                max: this.midiToNote(maxMidi)
            };
        },
        
        calculateAverageDifficulty(songs) {
            if (songs.length === 0) return 'beginner';
            
            const difficultyValues = {
                'beginner': 1,
                'elementary': 2,
                'intermediate': 3,
                'advanced': 4,
                'expert': 5
            };
            
            const average = songs.reduce((sum, song) => {
                return sum + (difficultyValues[song.difficulty] || 1);
            }, 0) / songs.length;
            
            const difficultyNames = Object.keys(difficultyValues);
            return difficultyNames[Math.round(average) - 1] || 'beginner';
        },
        
        // Utility methods
        
        noteToMIDI(note) {
            const noteMap = {
                'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3,
                'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'G#': 8,
                'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11
            };
            
            const match = note.match(/([A-G][#b]?)(\d+)/);
            if (!match) return 60;
            
            const noteName = match[1];
            const octave = parseInt(match[2]);
            
            return (octave * 12) + noteMap[noteName] + 12;
        },
        
        midiToNote(midi) {
            const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
            const octave = Math.floor((midi - 12) / 12);
            const noteIndex = (midi - 12) % 12;
            return noteNames[noteIndex] + octave;
        },
        
        // Song loading and conversion
        
        loadSong(songId) {
            const song = this.getSong(songId);
            if (!song) return null;
            
            // Convert to engine format
            return this.convertSongToEngineFormat(song);
        },
        
        convertSongToEngineFormat(song) {
            return {
                keySignature: song.keySignature,
                timeSignature: song.timeSignature,
                tempo: song.tempo,
                clef: song.staffType || 'grand',
                measures: song.measures.map(measure => this.convertMeasure(measure)),
                metadata: {
                    title: song.title,
                    composer: song.composer,
                    difficulty: song.difficulty,
                    category: song.category,
                    description: song.description,
                    tags: song.tags
                }
            };
        },
        
        convertMeasure(measure) {
            const convertedMeasure = {
                timeSignature: measure.timeSignature,
                notes: [],
                chords: [],
                totalBeats: measure.totalBeats
            };
            
            // Convert notes
            if (measure.notes) {
                convertedMeasure.notes = measure.notes.map(note => ({
                    ...note,
                    midi: this.noteToMIDI(note.pitch + note.octave),
                    staff: this.determineNoteStaff(note),
                    accidental: this.determineAccidental(note),
                    noteType: this.durationToNoteType(note.duration),
                    dotted: this.isDotted(note.duration)
                }));
            }
            
            // Convert chords
            if (measure.chords) {
                convertedMeasure.chords = measure.chords.map(chord => ({
                    ...chord,
                    notes: chord.notes.map(note => ({
                        ...note,
                        midi: this.noteToMIDI(note.pitch + note.octave),
                        staff: this.determineNoteStaff(note),
                        accidental: this.determineAccidental(note),
                        noteType: this.durationToNoteType(note.duration),
                        dotted: this.isDotted(note.duration)
                    }))
                }));
            }
            
            return convertedMeasure;
        },
        
        determineNoteStaff(note) {
            const midi = this.noteToMIDI(note.pitch + note.octave);
            return midi >= 60 ? 'treble' : 'bass';
        },
        
        determineAccidental(note) {
            if (note.pitch.includes('#')) return 'sharp';
            if (note.pitch.includes('b')) return 'flat';
            return null;
        },
        
        durationToNoteType(duration) {
            if (duration >= 4) return 'whole';
            if (duration >= 2) return 'half';
            if (duration >= 1) return 'quarter';
            if (duration >= 0.5) return 'eighth';
            if (duration >= 0.25) return 'sixteenth';
            return 'thirty-second';
        },
        
        isDotted(duration) {
            const dottedDurations = [1.5, 0.75, 0.375, 0.1875, 3];
            return dottedDurations.includes(duration);
        },
        
        // Export methods for integration
        
        exportSongList() {
            return Object.values(this.songs).map(song => ({
                id: song.id,
                title: song.title,
                composer: song.composer,
                category: song.category,
                difficulty: song.difficulty,
                tempo: song.tempo,
                description: song.description,
                tags: song.tags
            }));
        },
        
        exportCategoryList() {
            return Object.entries(this.songCategories).map(([key, name]) => ({
                id: key,
                name: name,
                songCount: this.getSongsByCategory(key).length
            }));
        },
        
        exportCollectionList() {
            return Object.entries(this.exerciseCollections).map(([key, collection]) => ({
                id: key,
                title: collection.title,
                description: collection.description,
                difficulty: collection.difficulty,
                songCount: collection.songs.length,
                estimatedTime: collection.estimatedTime,
                tags: collection.tags
            }));
        },
        
        // Integration with main game engine
        
        generatePlaylist(criteria) {
            let songs = Object.values(this.songs);
            
            // Apply filters
            if (criteria.difficulty) {
                songs = songs.filter(song => song.difficulty === criteria.difficulty);
            }
            
            if (criteria.category) {
                songs = songs.filter(song => song.category === criteria.category);
            }
            
            if (criteria.maxDuration) {
                songs = songs.filter(song => {
                    const duration = this.calculateSongDuration(song);
                    return duration <= criteria.maxDuration;
                });
            }
            
            if (criteria.keySignature) {
                songs = songs.filter(song => song.keySignature === criteria.keySignature);
            }
            
            if (criteria.tags) {
                songs = songs.filter(song => 
                    criteria.tags.some(tag => song.tags.includes(tag))
                );
            }
            
            // Sort and limit
            if (criteria.randomize) {
                songs = songs.sort(() => 0.5 - Math.random());
            } else {
                songs = songs.sort((a, b) => a.title.localeCompare(b.title));
            }
            
            if (criteria.limit) {
                songs = songs.slice(0, criteria.limit);
            }
            
            return {
                songs: songs,
                totalSongs: songs.length,
                totalDuration: songs.reduce((sum, song) => sum + this.calculateSongDuration(song), 0),
                averageDifficulty: this.calculateAverageDifficulty(songs)
            };
        }
    };
    
    // Initialize when ready
    $(document).ready(function() {
        window.SightReadingSongsLibrary.init();
        
        // Make available globally for other scripts
        if (window.sightReadingEngine) {
            window.sightReadingEngine.songsLibrary = window.SightReadingSongsLibrary;
        }
    });

})(jQuery);