#!/usr/bin/env rakudo

constant FLAGS = "ovufwe";
# o : Show obsolete strings
# v : Show vanished strings
# u : Show unfinished strings
# f : Show finished strings
# w : Show untranslated (Without translation) strings
# e : Show empty strings

constant FIELDS = "cst";
# c : Show context fields
# s : Show source fields
# t : Show translation fields

subset flags of Str where *.comb (<=) FLAGS.comb ;
subset fields of Str where *.comb (<=) FIELDS.comb ;

sub MAIN(
    Str $file,               #= .ts file to analyse
    Bool :s($stats) = False, #= Show stats
    flags :$flags = "ovufwe",    #= List flags among "ovufwe"
    fields :$fields = "cs",  #= List of showed fields among "cst"
    Str :$cfile = "",        #= File with contexts to display
) {

    doTheWork(file => $file, cfile => $cfile,
             flags => $flags, fields => $fields, stats => $stats);
}

sub USAGE
{

    say q:to/END/;

        This script is a tool to show quickly some informations about the translations a .ts file contains.

        Usage :

        raku scripts/show-ts.raku [-s] [--flags=<flags>] \
                                    [--fields=<fields>] [--cfile=<Str>] <file>

            <file>               .ts file to analyse

            -s                   Show stats [default: False]

            --flags=<flags>      List flags among "ovufwe" [default: 'ovufwe']
                                    o : Show Obsolete strings
                                    v : Show Vanished strings
                                    u : Show Unfinished strings
                                    f : Show Finished strings
                                    w : Show untranslated strings
                                        (Without translation)
                                    e : Show Empty strings

            --fields=<fields>    List of showed fields among "cst" [default: 'cs']
                                    c : Show Context fields
                                    s : Show Source fields
                                    t : Show Translation fields

            --cfile=<Str>        File with contexts to display [default: '']
                                   If this file is specified, only the contexts
                                   listed inside it are displayed.
                                   The contexts have only to be listed, separated with blanks, tabs and/or line feed.


        Examples :

            raku scripts/show-ts.raku -s data/locale/es.ts

            raku scripts/show-ts.raku --flags=uwe --cfile=A.txt  data/locale/es.ts

        END
}


sub getFlags(Str $allData, Str $value --> List(Bool))
{
    my @in = $value.comb;
    my Bool @result;
    for $allData.comb -> $v {
        @result.push: ($v (elem) @in);
    }
    return @result;
}

class Message
{
    has Str $.source;
    has Str $.translation;
    has Str $.flag = "";
}

class Statistics
{
    has Int $.nbMessage is rw = 0;
    has Int $.nbSource is rw = 0;

    has Int $.nbUnfinished is rw = 0;
    has Int $.nbObsolete is rw = 0;
    has Int $.nbVanished is rw = 0;
    has Int $.nbOthers is rw = 0;
    has Int $.nbOK is rw = 0;

    has Int $.nbUntranslated is rw = 0;
    has Int $.nbEmpty is rw = 0;
}


sub doTheWork(
    Str :$file,  #= .ts file to analyse
    Str :$cfile, #= Only contexts listed here have to be displayed
    Str :$flags,
    Str :$fields,
    Bool :$stats, )
{
    my Bool ($obsolete, $vanished, $unfinished, $finished,
             $untranslated, $empty, $all) = getFlags(FLAGS, $flags);
    my Bool ($fcontext, $fsource,
             $ftranslation) = getFlags(FIELDS, $fields);

    if $all {
        $obsolete = True;
        $vanished = True;
        $unfinished = True;
        $finished = True;
        $untranslated = True;
        $empty = True;
    }

    my @selectedContexts;
    if $cfile {
        my $text = slurp $cfile;
        my $m = $text ~~ m:g/\s*(\S+)\s*/;
        for $m.list -> $v {
            @selectedContexts.push: $v[0].Str;
        }
    }

    my $txt = slurp $file;

    my %messages;   # %messages{"context"} = list of Message
    my %stats;      # %stats{"context"} = Statistic
    my Int $nbContext = 0;

    my $c = $txt ~~ m:g/'<context>' \s*? '<name>' (.*?) '</name>' (.*?) '</context>'/;

    # say "Elems = ", $c.elems;

    for $c.list -> $k {
        my Str $context = $k[0].Str;
        my Str $ContextString = $k[1].Str;

        # say $context;
        $nbContext++;

        if %stats{$context}:!exists {
            %stats{$context} = Statistics.new;
        }
        my $statistic = %stats{$context};

        # Some messages have the form: '<message numerus="yes"> ...'
        my $r = $ContextString ~~ m:g/'<message' .*? '>' (.*?) '</message>'/;

        # say "Elems = ", $r.elems;

        for 0..* Z $r.list -> ($i, $e) {
        #     say "";
        #     say "$i :";
        #     say $e[0];
            %stats{$context}.nbMessage++;

            if $e[0] ~~ m/'<source>' (.*?) '</source>'/ {
        #         say "$i : Source = $0";
                %stats{$context}.nbSource++;

                my Str $source = $0.Str;
                my Str $message = $e[0].Str;
                my Str $status;
                my Str $translation;

                if $e[0] ~~ m/'<translation type="' (.*?) '">'
                                            (.*?) '</translation>'/ {
                    $status = $0.Str;
                    $translation = $1.Str;

                    given $status {
                        when "unfinished" {
                            %stats{$context}.nbUnfinished++;
                        }
                        when "obsolete" {
                            %stats{$context}.nbObsolete++;
                        }
                        when "vanished" {
                            %stats{$context}.nbVanished++;
                        }
                        default {
                            %stats{$context}.nbOthers++;
                        }
                    }

                } elsif $e[0] ~~ m/'<translation>' (.*?) '</translation>'/  {
                    $status = "OK";
                    %stats{$context}.nbOK++;
                    $translation = $0.Str;
                } else {
                    die "Abnormal message :\n$message\n";
                }

                %stats{$context}.nbEmpty++ if $source ~~ "";
                %stats{$context}.nbUntranslated++
                                            if $translation ~~ "";

                %messages{$context}.push: Message.new(
                                source => $source,
                                translation => $translation,
                                flag => $status,
                );

            }


        }

    }


    for %messages.keys.sort -> $k {
        my $context = $k;
        next if $cfile && $context !(elem) @selectedContexts;
        for @(%messages{$k}) -> $m {
            my Str $pflag = "";
            $pflag ~= "E" if $m.source ~~ "";  # Empty
            $pflag ~= "U" if $m.translation ~~ "";  # Untranslated
            $pflag ~= " " ~ $m.flag;

            if (    ($vanished && $m.flag ~~ "vanished")
                         || ($unfinished && $m.flag ~~ "unfinished")
                         || ($obsolete && $m.flag ~~ "obsolete")
                         || ($finished && $m.flag ~~ "OK")
                         || ($untranslated && $m.translation ~~ "")
                         || ($empty && $m.source ~~ "")
               )
                {
                    print setSize($context, 25), "|" if $fcontext;
                    print setSize($pflag, 10);
                    print "|", setSize($m.source, 40) if $fsource;
                    print "|", setSize($m.translation, 40)
                                                    if $ftranslation;
                    say "";
                }
        }
    }

    sub sayHeader {
        say "";
        say "  Msg   Src    Unfin  Obsol Vanish Others    OK   Empty Untran    Context ";
        say "------ ------ ------ ------ ------ ------ ------ ------ ------ --------------------";
    }

    if $stats {
        say "";
        say "File : $file";
        say "   Number of contexts : $nbContext";

        for 0..Inf Z %stats.keys.sort -> ($i, $k) {
            say sprintf("%6d " x 9 ~ " %s",
                         %stats{$k}.nbMessage,
                         %stats{$k}.nbSource,
                         %stats{$k}.nbUnfinished,
                         %stats{$k}.nbObsolete,
                         %stats{$k}.nbVanished,
                         %stats{$k}.nbOthers,
                         %stats{$k}.nbOK,
                         %stats{$k}.nbEmpty,
                         %stats{$k}.nbUntranslated,
                         $k);
             sayHeader if $i % 30 == 0;
        }

        sayHeader;
        say sprintf("%6d " x 9 ~ " %s",
                        ([+] %stats.values>>.nbMessage),
                        ([+] %stats.values>>.nbSource),
                        ([+] %stats.values>>.nbUnfinished),
                        ([+] %stats.values>>.nbObsolete),
                        ([+] %stats.values>>.nbVanished),
                        ([+] %stats.values>>.nbOthers),
                        ([+] %stats.values>>.nbOK),
                        ([+] %stats.values>>.nbEmpty),
                        ([+] %stats.values>>.nbUntranslated),
                        "Total of all contexts");
    }

}

# Replace every "\n" in $txt with "€"
# Then, if the length of $txt is > $size, truncate it
# Else, pad $txt with blanks until its length is $size
sub setSize(Str $txt is copy, Int $size --> Str)
{
    if $txt.chars > $size {
        $txt .= substr: 0, $size;
    } else {
        $txt ~= " " x ($size - $txt.chars);
    }

    $txt ~~ s:g/ "\n" /€/;

    return $txt;
}
