% This LilyPond file was generated by Rosegarden 25.06
\include "nederlands.ly"
\version "2.18.0"
\header {
    copyright = "Unknown"
    subtitle = "blank"
    title = "Untitled"
    tagline = "Created using Rosegarden 25.06 and LilyPond"
}
#(set-global-staff-size 18)
#(set-default-paper-size "a4")
global = { 
    \time 4/4
    \skip 1*4 
}
globalTempo = {
    \override Score.MetronomeMark.transparent = ##t
    \tempo 4 = 100  
}
\score {
    << % common
        % Force offset of colliding notes in chords:
        \override Score.NoteColumn.force-hshift = #1.0
        % Allow fingerings inside the staff (configured from export options):
        \override Score.Fingering.staff-padding = #'()

        \context Staff = "track 1, Velocity interpretation" << 
            \set Staff.instrumentName = \markup { \center-column { "Velocity interpretation " } }
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

                \clef "treble"
                c' 8 -\pp [ d' \< e' f' ] g' [ a' b' c'' ] 
                % warning: overlong bar truncated here |
                d'' 8 [ e'' f'' g'' ] a'' [ b'' c''' -\ff \! b'' \> ] 
                % warning: overlong bar truncated here |
                a'' 8 [ g'' f'' e'' ] d'' [ c'' b' a' ]  |
                g' 8 [ f' e' d' ] c' 2 -\pp \! 
                \bar "|."
            } % Voice

            % End of segment Acoustic Grand Piano

            % End voice 0
        >> % Staff ends

        \context Staff = "track 2, Articulation interpretation" << 
            \set Staff.instrumentName = \markup { \center-column { "Articulation interpretation " } }
            \set Staff.midiInstrument = ""
            \set Score.skipBars = ##t
            \set Staff.printKeyCancellation = ##f
            \new Voice \global
            \new Voice \globalTempo
            \set Staff.autoBeaming = ##f % turns off all autobeaming

            \context Voice = "voice 1.0" {
                % Segment: General MIDI Device #2
                \override Voice.TextScript.padding = #2.0
                \override MultiMeasureRest.expand-limit = 1

                \clef "treble"
                \clef "treble"
                c' 8 -\pp -\tenuto [ d' -\tenuto \< e' -\tenuto f' -\tenuto ] g' -\staccato [ a' -\staccato b' -\staccato c'' -\staccato ] 
                % warning: overlong bar truncated here |
                d'' 8 -\tenuto [ e'' -\tenuto f'' -\tenuto g'' -\tenuto ] a'' [ b'' c''' -\ff -\accent \! b'' \> ] 
                % warning: overlong bar truncated here |
                a'' 8 -\tenuto [ g'' -\tenuto f'' -\staccato e'' -\staccato ] d'' -\tenuto [ c'' -\tenuto b' -\staccato a' -\staccato ]  |
                g' 8 -\staccato [ f' -\staccato e' -\staccato d' -\staccato ] c' 2 -\pp \! 
                \bar "|."
            } % Voice

            % End of segment General MIDI Device #2

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
