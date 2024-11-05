

#`{{

After some new translation contexts have been added (as INSTRUMENT, COLOUR, etc.),
translations in these new contexts are flagged "unfinished" while the saùe translations in the old contexts are flagged "vanished" (or "obsolete").

This script copy the translation flags from the previous .ts file to the
translations in the new context and remove the translations from the oldContextcontext.

For working this scripts needs :
 - The old .ts files (where it can find the old flags) : they are in $oldLocaleDir
 - The new modified .ts files (to be changed) : they are in $localeDir
 - A list of the new contexts and their associated translations : $info
 - The name of the oldContext of all the concerned translations : $oldContext

This script should be run only once. So these data are hardcoded below.

}}


my $localeDir = "data/locale";        # Directory of the .ts files
my $oldLocaleDir = "data/oldLocale";  # Directory of the old .ts files
my $info = "data/new_contexts.txt";   # The new contexts file

my $oldContext = "QObject";     # Old context name of the translations

my Str $err = "";   # String where errors messages are logged

# First step : Read the new contexts file and store its content in %modified

# Hash of modified contexts
# %modified<new_context> = ( list of translation sources )
my %modified;

{
    say 'Read the "new contexts" file';
    my $txt = slurp $info;

    # $txt.lines>>.say;

    for $txt.lines -> $l {
        $l ~~ m/^(\S*) \s+ (.*) $/;

        #  $0 : context
        #  $1 : translation source

        say "WARNING: Empty context  \>$0\< \>$1\<" if !$0;
        say "WARNING: Empty source  \>$0\< \>$1\<" if !$1;

        # Restore new line characters if any
        my $ctx = $0.Str;
        my $src = $1.Str;
        $ctx ~~ s:g/ '€' /\n/;
        $src ~~ s:g/ '€' /\n/;

        %modified{$ctx}.push: toXml $src;
    }

    say "{$txt.lines.elems} translations in {%modified.keys.elems} contexts";

    # Remove duplicated translations
    for %modified.keys -> $k {
        %modified{$k} = (set @(%modified{$k})).keys.sort;
    }

    say "{[+] @(%modified.values)>>.elems} translations in {%modified.keys.elems} contexts";
}

# say "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
# say "Print out %modified for test";
# say "";
# %modified.keys.sort>>.&{
#     %modified{$_}.sort>>.&{ say '"', $OUTER::_, '": "', $_, '"' }
# };
# say "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++";

say "";


my @tsFiles = dir $localeDir, test => / '.ts' $ /;

# @tsFiles = ( "data/locale/fr.ts".IO, );   # for TEST


# Process each .ts file
my $nbf = @tsFiles.elems;
FILE:
for 1..* Z @tsFiles -> ($j, $file) {
    say "Processing $file   ($j/$nbf)";
    $err ~= "Processing $file" ~ "\n";

    # Get the associated old .ts file
    # (used as reference to know the already translated strings)
    my $refFile = $oldLocaleDir ~ '/' ~ $file.basename;

    # Read the .ts file and the reference .ts file
    my $tsTxt = slurp $file;
    my $refTsTxt = slurp $refFile;

    # Get the reference context part from $refTsTxt
    my $refContextField = getContext $refTsTxt, $oldContext;
    if !$refContextField {
        msg "Can't found context \"$oldContext\" in \"$refFile\"";
        next FILE;
    }

    # Get the old context part from $tsTxt
    my $oldContextField = getContext $tsTxt, $oldContext;

    # Replace the found text with a marker
    my $oldContextMarker = "___{$oldContext}___";
    if $oldContextField {
        $tsTxt ~~ s/$oldContextField/$oldContextMarker/;
    } else {
        msg "Can't found context \"$oldContext\" in \"$file\"";
        next FILE;
    }

    # Process each new context
    for %modified.keys.sort -> $newContext {
        say "";
        say "    Processing new context \"$newContext\"";
        # Get the context part from $tsTxt then replace it with a marker

        # Get the context field
        my Str $contextField = getContext $tsTxt, $newContext;

        # Replace the found text with a marker
        my $contextMarker = "___{$newContext}___";
        if $contextField {
            $tsTxt ~~ s/$contextField/$contextMarker/;
        } else {
            msg "Can't found context \"$newContext\" in \"$file\"";
            next FILE;
        }

        # Process each tranlation
        my $nbt = @(%modified{$newContext}).elems;
        TRANSLATION:
        for 1..* Z @(%modified{$newContext}).sort -> ($i, $source) {
            print "        Processing source message \"$source\"   ($i/$nbt)\r";

            # Get translation in $contextField
            my $message = getMessage $contextField, $source;
            if !$message {
                say "";
                msg "Source \"$source\" not found in \"$newContext\" context";
                msg "    ==> Translation skipped";
                next TRANSLATION;
            }

            # Remove the translation from the $oldContextField
            my $oldMessage = getMessage $oldContextField, $source;
            if $oldMessage {
                $oldContextField ~~ s/$oldMessage//;
            } else {
                say "";
                msg "Source \"$source\" not found in \"$oldContext\" context";
                msg "    ==> Translation not removed";
            }

            # Get translation in $refContextField
            my $refMessage = getMessage $refContextField, $source;
            if !$refMessage {
                say "";
                msg "Source \"$source\" not found in reference context";
                msg "    ==> Translation kept unfinished";
                next TRANSLATION;
            }

            # Get the status of this translation
            my Str $status;
            if $refMessage ~~ m/'<translation type="unfinished">'/ {
                $status = "unfinished";
            } elsif    $refMessage ~~ m/'<translation type="obsolete">'/
                    || $refMessage ~~ m/'<translation type="vanished">'/ {
                $status = "obsolete";
            } elsif $refMessage ~~ m/'<translation>'/ {
                $status = "OK";
            } else {
                say "";
                msg "WARNING: Translation reference status is unknown.";
                msg $refMessage;
                msg "";
            }

            # If the reference status is OK, set the new status OK
            if $status ~~ "OK" {
                my Str $newMessage = $message;
                if $newMessage !~~ s/ '<translation' .*? '>'/<translation>/ {
                    say "";
                    msg "ERROR?: translation field not found in message";
                    msg $message;
                    msg "";
                }
                $contextField ~~ s/$message/$newMessage/;
            } # else keep the message field as is
        }

        # Move back the modified context field in the text
        $tsTxt ~~ s/$contextMarker/$contextField/;
    }

    # Move back the modified old context field in the text
    $tsTxt ~~ s/$oldContextMarker/$oldContextField/;

    # Write back the modified file
    say "";
    msg "Write back $file";
    spurt $file, $tsTxt;
    msg "";
}

# Log errors
spurt "fix-ts-errors.log", $err;

# Display a message and add it to the global string $err
sub msg(Str $txt) {
    say $txt;
    $err ~= $txt ~ "\n";
}

# Convert source special characters to XML
sub toXml(Str $s is copy --> Str)
{
    $s ~~ s:g/ '&' /&amp;/;
    $s ~~ s:g/ "'" /&apos;/;
    $s ~~ s:g/ '>' /&gt;/;
    $s ~~ s:g/ '<' /&lt;/;
    $s ~~ s:g/ '"' /&quot;/;

    return $s;
}

sub getPattern(Str $text, Str $outerMarker, Str $innerMarker, Str $target --> Str)
{
    my Str $pattern = "";
    my $r = $text ~~ m:g/('<' $outerMarker '>' .*? '</' $outerMarker '>')/;
    for $r.list -> $e {
        if $e[0] ~~ m/'<' $innerMarker '>' $target '</' $innerMarker '>'/ {
            $pattern = $e[0].Str;
            last;
        }
    }
    return $pattern;
}

sub getContext(Str $text, Str $name --> Str)
{
    getPattern $text, "context", "name", $name;
}

sub getMessage(Str $text, Str $source --> Str)
{
    getPattern $text, "message", "source", $source;
}

