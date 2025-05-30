% This LilyPond file was generated by Rosegarden 25.06
\include "nederlands.ly"
\version "2.18.0"
\header {
    title = "Testcase for grace notes and ties"
    tagline = "Created using Rosegarden 25.06 and LilyPond"
}
#(set-global-staff-size 18)
#(set-default-paper-size "a4")
global = { 
    \time 4/4
    \skip 1*3 
}
globalTempo = {
    \override Score.MetronomeMark.transparent = ##t
    \tempo 4 = 120  
}
\score {
    << % common
        % Force offset of colliding notes in chords:
        \override Score.NoteColumn.force-hshift = #1.0
        % Allow fingerings inside the staff (configured from export options):
        \override Score.Fingering.staff-padding = #'()

        \context Staff = "track 1" << 
            \set Staff.midiInstrument = "Acoustic Grand Piano"
            \set Score.skipBars = ##t
            \set Staff.printKeyCancellation = ##f
            \new Voice \global
            \new Voice \globalTempo
            \set Staff.autoBeaming = ##f % turns off all autobeaming

            \context Voice = "voice 0.0" {
                % Segment: Acoustic Grand Piano
                \override Voice.TextScript.padding = #2.0
                \override MultiMeasureRest.expand-limit = 1
                \once \override Staff.TimeSignature.style = #'numbered 
                \time 4/4
                
                \clef "treble"
                \key c \major
                a' 4 \grace { e' 16 } f' 4 \grace { f' 8 [ fis' ] } g' 2  |
                \grace { \times 2/3 { \stemUp b'' 16 [ a'' g'' ] } } \stemNeutral e'' 4 r \stemUp < c'' e'' > 8 [ \grace { < bes' d'' > } < b' d'' > ] r4 \stemNeutral  |
                \stemUp f'' 4 \grace { < des'' f'' > 8 } < d'' f'' > 4 \grace { \stemNeutral e' 16 } f' 4 r 
                \bar "|."
            } % Voice

            % End of segment Acoustic Grand Piano

            % End voice 0
        >> % Staff ends

    >> % notes

    \layout {
        \context { \Staff \RemoveEmptyStaves }
        \context { \GrandStaff \accepts "Lyrics" }
    }
%     uncomment to enable generating midi file from the lilypond source
%         \midi {
%         } 
} % score
