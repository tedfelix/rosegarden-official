% This LilyPond file was generated by Rosegarden 25.06
\include "nederlands.ly"
\version "2.18.0"
\header {
    composer = "D. Michael McIntyre"
    copyright = "2004"
    title = "Perfect Moment"
    tagline = "Created using Rosegarden 25.06 and LilyPond"
}
#(set-global-staff-size 18)
#(set-default-paper-size "a4")
global = { 
    \time 4/4
    \skip 1*100 
}
globalTempo = {
    \override Score.MetronomeMark.transparent = ##t
    \tempo 4 = 110  
}
\score {
    << % common
        % Force offset of colliding notes in chords:
        \override Score.NoteColumn.force-hshift = #1.0
        % Allow fingerings inside the staff (configured from export options):
        \override Score.Fingering.staff-padding = #'()

        \context Staff = "track 1, XSynth (whole notes)" << 
            \set Staff.instrumentName = \markup { \center-column { "XSynth (whole notes) " } }
            \set Staff.midiInstrument = ""
            \set Score.skipBars = ##t
            \set Staff.printKeyCancellation = ##f
            \new Voice \global
            \new Voice \globalTempo
            \set Staff.autoBeaming = ##f % turns off all autobeaming

            \context Voice = "voice 0.0" {
                % Segment: low bass (step recorded)
                \override Voice.TextScript.padding = #2.0
                \override MultiMeasureRest.expand-limit = 1

                \repeat unfold 25 {
                    \clef "bass"
                    g, 1  |
                    f, 1  |
                    ees, 1  |
                    d, 1 
                } % close repeat
            } % Voice

            % End of segment low bass (step recorded)

            % End voice 0
        >> % Staff ends

        \context Staff = "track 2, Hexeter (eighth notes)" << 
            \set Staff.instrumentName = \markup { \center-column { "Hexeter (eighth notes) " } }
            \set Staff.midiInstrument = ""
            \set Score.skipBars = ##t
            \set Staff.printKeyCancellation = ##f
            \new Voice \global
            \new Voice \globalTempo
            \set Staff.autoBeaming = ##f % turns off all autobeaming

            \context Voice = "voice 1.0" {
                % Segment: high bass (step recorded)
                \override Voice.TextScript.padding = #2.0
                \override MultiMeasureRest.expand-limit = 1

                \repeat unfold 25 {
                    \clef "bass"
                    g 8 -\staccato [ g -\staccato g -\staccato g -\staccato ] g -\staccato [ g -\staccato g -\staccato g -\staccato ]  |
                    f 8 -\staccato [ f -\staccato f -\staccato f -\staccato ] f -\staccato [ f -\staccato f -\staccato f -\staccato ]  |
                    ees 8 -\staccato [ ees -\staccato ees -\staccato ees -\staccato ] ees -\staccato [ ees -\staccato ees -\staccato ees -\staccato ]  |
                    d 8 -\staccato [ d -\staccato d -\staccato d -\staccato ] d -\staccato [ d -\staccato d -\staccato d -\staccato ] r16 
                    % warning: overlong bar truncated here
                } % close repeat
            } % Voice

            % End of segment high bass (step recorded)

            % End voice 0
        >> % Staff ends

        \context Staff = "track 3, XSynth (lead)" << 
            \set Staff.instrumentName = \markup { \center-column { "XSynth (lead) " } }
            \set Staff.midiInstrument = ""
            \set Score.skipBars = ##t
            \set Staff.printKeyCancellation = ##f
            \new Voice \global
            \new Voice \globalTempo
            \set Staff.autoBeaming = ##f % turns off all autobeaming

            \context Voice = "voice 2.0" {
                % Segment: Improv Oboe noodle  (recorded) (notation quantized, but not well-notated)
                \override Voice.TextScript.padding = #2.0
                \override MultiMeasureRest.expand-limit = 1

                \skip 1*4 
%% 5
                \clef "treble"
                \key g \minor
                g' 2. -\tenuto < a' bes' > 4 -\tenuto-\staccato  |
                c'' 2 -\tenuto d'' 4.. -\tenuto r16  |
                bes' 2.. -\tenuto a' 16 -\tenuto g' -\tenuto  |
                a' 1  |
                g' 2. -\tenuto a' 8 -\tenuto bes' -\tenuto  |
%% 10
                c'' 2 -\tenuto d''  |
                bes' 2. -\tenuto a' 8 -\tenuto [ g' -\tenuto ]  |
                a' 1  |
                d'' 2. -\tenuto bes' 4 -\tenuto  |
                c'' 2 -\tenuto ees''  |
%% 15
                f'' 2. -\tenuto d'' 4 -\tenuto  |
                c'' 1 -\tenuto  |
                g' 2. -\tenuto f' 4 -\tenuto  |
                ees' 2 -\tenuto d' 4 -\tenuto f' -\tenuto  |
                r2 r4 r8 r16 < ees' d' > -\staccato _~  |
%% 20
                d' 1 _~  |
                g' 2 bes' 4 -\tenuto a' -\tenuto  |
                c'' 4 -\tenuto bes' -\tenuto a' -\tenuto bes' -\tenuto  |
                g' 4 -\tenuto a' -\tenuto bes' -\tenuto a' -\tenuto  |
                f' 4 -\tenuto g' -\tenuto a' -\tenuto g' -\staccato  |
%% 25
                g' 4 bes' -\tenuto c'' -\tenuto d'' -\tenuto  |
                ees'' 4 -\tenuto bes' c'' -\tenuto d'' -\tenuto  |
                bes' 4 -\tenuto a' -\tenuto bes' -\tenuto c'' -\tenuto  |
                g' 4 -\tenuto bes' a' 2 -\tenuto  |
                g' 2. -\tenuto a' 8 -\tenuto bes' -\tenuto  |
%% 30
                c'' 2 -\tenuto d'' 4 bes' -\tenuto  |
                c'' 2. g' 8 -\tenuto a' -\tenuto  |
                < bes' a' > 4 -\tenuto-\staccato g' -\tenuto f' -\tenuto g'  |
                bes' 2. -\tenuto g' 8 -\tenuto [ a' -\tenuto ]  |
                bes' 2 g' 4. -\tenuto r8  |
%% 35
                a' 1  |
                g' 2. -\tenuto ees' 4 -\tenuto  |
                f' 2 -\tenuto d' -\tenuto  |
                ees' 2 -\tenuto c' -\tenuto  |
                d' 2 -\tenuto bes  |
%% 40
                c' 2. -\tenuto g' 4 -\tenuto  |
                f' 4 -\tenuto < ees' f' > -\tenuto-\staccato g' -\tenuto a'  |
                bes' 4 -\tenuto g' 8 -\tenuto [ a' -\tenuto ] bes' 4 -\tenuto c'' -\staccato  |
                d'' 4 -\tenuto ees'' -\tenuto f'' -\tenuto < g'' f'' > -\tenuto-\staccato  |
                ees'' 8 -\tenuto d'' -\tenuto [ c'' -\tenuto ] d'' -\tenuto ees'' 4 -\tenuto d'' -\staccato  |
%% 45
                g' 8 -\tenuto a' -\tenuto [ bes' -\tenuto ] c'' -\tenuto d'' 4 bes' 8 -\tenuto [ c'' ]  |
                < bes' a' > 4 -\tenuto-\staccato g' 8 -\tenuto bes' -\tenuto a' -\tenuto c'' 16 a' -\staccato c'' 4 -\tenuto  |
                < bes' c'' > 8 -\tenuto-\staccato r c'' 16 -\tenuto bes' 8 -\tenuto r16 a' 4 -\tenuto bes' 8 -\tenuto [ a' ]  |
                g' 8 -\tenuto bes' -\staccato bes' -\tenuto a' bes' 4. r8  |
                ees'' 16. -\staccato bes' 4 r32 d'' 16. ^( r32 ) c'' 4 bes' 8 -\staccato [ d'' -\tenuto _~ ]  |
%% 50
                d'' 8 -\tenuto-\staccato _~ bes' -\tenuto-\staccato ees'' -\tenuto-\staccato d'' -\staccato c'' 16 -\tenuto bes' 16. _( r32 r16 ) a' -\tenuto bes' 8 -\staccato r16  |
                g' 4 bes' 8 -\staccato a' 4 f' 8 -\staccato g' 16 -\staccato r a' 8 -\staccato  |
                bes' 4 a' 8. r16 g' 4. -\tenuto r8  |
                g' 4 -\tenuto r8 bes' 4 -\tenuto r8 c'' 4 -\staccato  |
                bes' 4 -\tenuto r8 a' 4 -\tenuto r8 bes' 4 -\staccato  |
%% 55
                c'' 4. a' g' 4  |
                fis' 4. g' 4 -\tenuto r8 a' 8. r16  |
                g' 4 -\tenuto r8 a' 4 -\tenuto r8 bes' 8. r16  |
                c'' 4 -\tenuto r8 bes' 4 -\tenuto r8 c'' 4  |
                d'' 4 -\tenuto r8 bes' 4. f'' 4  |
%% 60
                g'' 4. f'' 2 r16.. 
                % warning: overlong bar truncated here
                % warning: bar too short, padding with rests
                % 226560 + 3808 < 230400  &&  63/64 < 4/4
                r64  |
                g' 4. -\tenuto g'' -\tenuto f'' 4 -\tenuto ) _~  |
                f'' 1 -\tenuto _~  |
                g' 4 -\tenuto r16 g'' 4. -\tenuto r16 f'' 4 -\tenuto  |
                ees'' 4 -\tenuto r16 d'' 2 -\tenuto r16 r8  |
%% 65
                g' 4. -\tenuto g'' 4 -\tenuto r16 f'' 4 -\tenuto r16  |
                ees'' 4 -\tenuto d'' 8 -\tenuto ees'' 4 -\tenuto f'' 8 -\tenuto [ ees'' -\tenuto ] d'' -\tenuto  |
                c'' 4 -\tenuto ees'' 8 -\tenuto [ d'' -\tenuto ] c'' 4 -\tenuto bes'  |
                c'' 1 -\tenuto  |
                g' 4 -\tenuto g'' 4. -\tenuto f'' 8 < ees'' d'' > 4 -\tenuto-\staccato  |
%% 70
                ees'' 8. -\tenuto d'' 8 -\tenuto r16 c'' 4 -\tenuto d'' 8 -\tenuto ees'' 4 -\tenuto  |
                g' 4 -\tenuto g'' 4. -\tenuto f'' 8 -\tenuto ees'' -\tenuto d'' -\tenuto  |
                ees'' 4 -\tenuto g'' f'' ees''  |
                c'' 4. -\tenuto ees'' 2 -\tenuto r8  |
                c'' 4 -\tenuto d'' 8 -\tenuto c'' 2 -\tenuto bes' 8 -\tenuto  |
%% 75
                c'' 4 -\tenuto bes' 8 -\tenuto a' 4 -\tenuto bes' 8 -\tenuto < c'' d'' > 4 -\tenuto-\staccato  |
                c'' 2. -\tenuto g' 4 -\tenuto  |
                ees'' 4. -\tenuto g'' -\tenuto < f'' ees'' > 4 -\tenuto-\staccato  |
                f'' 4 -\tenuto g'' 4.. -\tenuto ees'' 4 -\tenuto r16  |
                g'' 4 -\tenuto a'' 8 bes'' 4. -\tenuto c''' 4 -\tenuto  |
%% 80
                bes'' 2. g'' 4 -\tenuto _~  |
                g'' 4. -\tenuto _~ ees'' a'' 8 -\tenuto g'' -\tenuto  |
                f'' 2. -\tenuto g'' 8 -\tenuto a'' ) ^(  |
                bes'' 4. -\tenuto c''' -\tenuto d''' 4  |
                c''' 2. -\tenuto g'' 4 -\staccato  |
%% 85
                fis'' 2. -\tenuto g'' 4 -\tenuto  |
                e'' 2 -\tenuto r8 r16 fis'' 4 -\tenuto r16  |
                d'' 2. -\tenuto < e'' fis'' > 4 -\tenuto-\staccato  |
                g'' 2. -\tenuto a'' 4  |
                g' 2. -\tenuto g'' 4  |
%% 90
                g' 2. -\tenuto g'' 4 -\tenuto  |
                < g' ees'' > 2. -\tenuto g'' 4  |
                f'' 1 -\tenuto  |
                g' 4 -\tenuto g'' 8 -\tenuto f'' 4. ees'' 8 -\tenuto d''  |
                ees'' 4 -\tenuto c'' 8 -\tenuto d'' -\tenuto ^( [ ees'' 8. ) ] r16 d'' 8 -\tenuto c'' -\tenuto  |
%% 95
                bes' 4 -\tenuto c'' 8 -\tenuto [ bes' -\tenuto ] a' -\staccato-\tenuto r16 bes' 4 r16  |
                bes' 4 -\tenuto a' 8 bes' 4 -\tenuto a' 8 -\tenuto g' 4  |
                bes' 4 -\tenuto c'' -\tenuto d'' -\tenuto bes' -\tenuto  |
                < ees'' ees'' > 8 -\tenuto-\staccato bes' -\tenuto d'' 4 -\tenuto c'' 4. bes' 8  |
                d'' 4 -\tenuto bes' 8 -\tenuto a' 4 -\tenuto bes' 8 a' 4 -\tenuto  |
%% 100
                < g' a' > 8 -\tenuto r r16 bes' 2 -\tenuto r16.. 
                % warning: bar too short, padding with rests
                % 380160 + 3540 < 384000  &&  59/64 < 4/4
                r64 r16 
                \bar "|."
            } % Voice

            % End of segment Improv Oboe noodle  (recorded) (notation quantized, but not well-notated)

            % End voice 0
        >> % Staff ends

        \context Staff = "track 4, Hexeter (flanged drums)" << 
            \set Staff.instrumentName = \markup { \center-column { "Hexeter (flanged drums) " } }
            \set Staff.midiInstrument = ""
            \set Score.skipBars = ##t
            \set Staff.printKeyCancellation = ##f
            \new Voice \global
            \new Voice \globalTempo
            \set Staff.autoBeaming = ##f % turns off all autobeaming

            \context Voice = "voice 3.0" {
                % Segment: Standard
                \override Voice.TextScript.padding = #2.0
                \override MultiMeasureRest.expand-limit = 1

                \repeat unfold 101 {
                    < c, aes, > 8 aes, 32 r aes, 16 r < e, aes, > 8 r32 r < aes, c, > 16 < c, aes, > 8 aes, 16 bes, r < e, aes, > 8 r32 r aes, 16 r 
                } % close repeat
            } % Voice

            % End of segment Standard

            % End voice 0
        >> % Staff ends

        \context Staff = "track 5, Hexeter (timpani)" << 
            \set Staff.instrumentName = \markup { \center-column { "Hexeter (timpani) " } }
            \set Staff.midiInstrument = ""
            \set Score.skipBars = ##t
            \set Staff.printKeyCancellation = ##f
            \new Voice \global
            \new Voice \globalTempo
            \set Staff.autoBeaming = ##f % turns off all autobeaming

            \context Voice = "voice 4.0" {
                % Segment: Timpani
                \override Voice.TextScript.padding = #2.0
                \override MultiMeasureRest.expand-limit = 1

                \repeat unfold 101 {
                    d 8 [ d 16 d ] d 64 r r32 r16 d r d r d r r8 d 16 r 
                } % close repeat
            } % Voice

            % End of segment Timpani

            % End voice 0
        >> % Staff ends

    >> % notes

    \layout {
        indent = 3.0\cm
        short-indent = 1.5\cm
        \context { \Staff \RemoveEmptyStaves }
        \context { \GrandStaff \accepts "Lyrics" }
    }
%     uncomment to enable generating midi file from the lilypond source
%         \midi {
%         } 
} % score
