% This LilyPond file was generated by Rosegarden 25.06
\include "nederlands.ly"
\version "2.18.0"
\header {
    copyright =  \markup { "Copyright "\char ##x00A9" xxxx Copyright Holder" }
    subtitle = "not yet subtitled"
    title = "Not Yet Titled"
    tagline = "Created using Rosegarden 25.06 and LilyPond"
}
#(set-global-staff-size 18)
#(set-default-paper-size "a4")
global = { 
    \time 4/4
    \skip 1*2 
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
                \key e \major
                cis' 4 fis' dis'' gis''  |
                cis'' 4 fis dis' fis'' 
            } % Voice

            % End of segment Acoustic Grand Piano

            \context Voice = "voice 0.0" {
                % Segment: Acoustic Grand Piano
                \override Voice.TextScript.padding = #2.0
                \override MultiMeasureRest.expand-limit = 1

                \skip 1*2 
                \clef "treble"
                cis' 4 fis' dis'' gis''  |
                cis'' 4 fis dis' fis'' 
            } % Voice

            % End of segment Acoustic Grand Piano

            \context Voice = "voice 0.0" {
                % Segment: Acoustic Grand Piano
                \override Voice.TextScript.padding = #2.0
                \override MultiMeasureRest.expand-limit = 1

                \skip 1*4 
%% 5
                \clef "treble"
                cis' 4 fis' dis'' gis''  |
                cis'' 4 fis dis' fis'' 
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
