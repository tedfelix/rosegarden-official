#!/usr/bin/env rakudo

# This script gathers and prints a summary of the LilyPond versions
# export test.
# It read on it's standard input the standard output of the
# versions_test_check_results.sh script.

my $text = $*IN.slurp(); # Read the whole standard input in $text

my %m;  # Main hash, key is version number

# Parse $text and gather all data in the hash of hashes %m
my $version;
for $text.lines -> $line {
    given $line {
        when / ^ '=======' \s+ (\d+ '.' \d+) / {
            $version = $0;
            %m{$version} = {};    # Create a subhash, key is file name
        }
        when / ^ (<-[:]>+ '.ly') ':' \s+ 'WARNING' \s+ (\d+) / {
            %m{$version}{$0} = "W$1";
        }
        when / ^ (<-[:]>+ '.ly') ':' \s+ 'ERROR' \s+ (\d+) / {
            %m{$version}{$0} = "E$1";
        }
        when / ^ (<-[:]>+ '.ly') ':' \s+ 'OK' / {
            %m{$version}{$0} = "OK";
        }
    }
}
# Now, %m{"version"}{"filename.ly"} = "OK" or "Ennn" (if errors found) or
# "Wnnn" (if warnings found) where nnn is the number of errors or warnings.

# A sub to center a text inside a column:
# add white spaces to $in until it reaches the size $size
sub center(Int $size, Str $in --> Str) {
    my $diff = $size - $in.chars;
    return $in if $diff <= 0;
    my $margin = ($diff/2).Int;
    my $out = " " x $margin ~ $in ~ " " x $margin;
    $out = " " ~ $out if $out.chars < $size;
    return $out;
}


my $size = 6;   # Size of a column in the output table

# %m.keys is an already existing set of versions

# Create a set of file names
my SetHash $files .= new;
for %m.keys -> $k {
    %m{$k}.keys>>.&{$files.set($_)};
}


# Print the table

my $header = [~] %m.keys.sort>>.&{center($size, $_)} >>~>> " ";
my $sepLine = ("-" x $size ~ " ") x %m.keys.elems;

say "";
say $header ~ "    Files";
say $sepLine ~ " " ~ "-" x ($files.keys>>.chars).max;
for $files.keys.sort -> $f {
    my Str $line;
    for %m.keys.sort -> $v {
        if %m{$v}{$f}:exists {
            $line ~= center($size, %m{$v}{$f}) ~ " ";
        } else {
            $line ~= " " x ($size + 1);
        }
    }
    say $line, " ", $f;
}
say "";

