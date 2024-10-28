
#`{{

Following a weird behaviour of Qt lupdate utility, all translations in
the "Rosegarden::StaffHeader" context have been accidentally moved in
the "StaffHeader" context.

For all .ts files, this script :
    - Remove the "Rosegarden::StaffHeader" context if it exists
    - Rename "Rosegarden::StaffHeader".the existing "StaffHeader" context

}}

my $localeDir = "data/locale";        # Directory of the .ts files
my $srcContext = "StaffHeader";       # New context name of the translations
my $dstContext = "Rosegarden::StaffHeader"; # Old context name of the translations


class Message
{
    has Str $.source;
    has Str $.translation;
    has Str $.flag = "";

    method isObsolete( --> Bool) { $.flag ~~ "obsolete" | "vanished" }
    method isUnfinished( --> Bool) { $.flag ~~ "unfinished" }
    method isOK( --> Bool) { $.flag ~~ "OK" }
    method dump(Str $s = "") {
        say "MSG ($s) : ", $.flag;
        say "   S = ", $.source;
        say "   T = ", $.translation;
    }
}

# Get the names of all the .ts files in the data/locale directory
my @tsFiles = dir $localeDir, test => / '.ts' $ /;


# Process each .ts file
FILE:
for @tsFiles -> $file {
    say "Processing $file";

    # Read the whole .ts file in a string
    my Str $tsTxt = slurp $file;

    # Process the old context part from the string
    my Str $dstContextField = getContext $tsTxt, $dstContext;
    if !$dstContextField {
        say "    Can't found context \"$dstContext\" in \"$file\"";
    } else {
        # Remove the context
        $tsTxt ~~ s/$dstContextField//;
        say "    \"$dstContext\" removed";
    }


    # Get the new context part from $tsTxt
    my Str $srcContextField = getContext $tsTxt, $srcContext;
    if !$srcContextField {
        say "    ERROR : Can't found context \"$srcContext\" in \"$file\"";
        next FILE;
    }

    # Replace in the file the whole context with a marker
    my $srcContextMarker = "___{$srcContext}___";
    $tsTxt ~~ s/$srcContextField/$srcContextMarker/;

    # Rename the context
    if $srcContextField ~~ s/ ('<context>' \s* '<name>') $srcContext '</name>'
                                                     /{$0}{$dstContext}<\/name>/ {
        say "    \"$srcContext\" context renamed \"$dstContext\"";
    } else {
        say "    ERROR : Can't rename \"$srcContext\" context";
        next FILE;
    }

    # Replace in the string the marker with the renamed context
    $tsTxt ~~ s/$srcContextMarker/$srcContextField/;

    # Write the modified file
    spurt $file, $tsTxt;

    say "    Done";
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


