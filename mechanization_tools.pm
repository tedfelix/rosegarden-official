
#
# mechanization_tools
#
# Copyright (c) 2008  Yves Guillemot <yc.guillemot@wanadoo.fr>
# Released under the GPL
#
#
# PURPOSE: To ease qt4 mechanised port by allowing the use of a kind of
# high level simple RegExp adapted to C++ code matching and replacement.
#
=USAGE

The "Process" package, called from the convert.pl file, allows to replace a
group of lines with another group.

The replacement is done only if the first line, called the "trigger" is found
in the text. Then others lines, called "targets" are also replaced.

The interesting points is that targets may be defined using keywords coming
from the trigger.

Here is an example :

  (1)  trigger :         '${x} = ${u} + ${u} ;'
  (2)    replacement :   '${x}=two*${u};'
  (3)    target :        '${y} is ${u} + ${x}'
  (4)      replacement : '${y} is three times ${u}'
  (5)      replacement : '${y} = three * ${x} / two;'
  (6)    target :        '${x} = ${u} + two * ${z} ;'
  (7)      replacement : '${z} = ${u};'

Line (1) defines the trigger. ${x} and ${u} are variables which may match any
word, but only a word. Of course ${x} and ${u} may match different words (while 
not necessarily different), but the two ${u} can only match the same word.
'+', '=' and ';' are "operators" and match themselves in the text.
Note that all elements in the trigger string shall be separated with white spaces.

In the processed text, this white spaces may be any number of spaces, tabulation
or newlines (as it may be in C).
There is no such rule in the replacement string, where the spacing used is
keeped.

If no line in the text has the form of the trigger, nothing more is done.

But, if such a line is found :
 - 1) this line is replaced by the line (2), keeping the ${x}
      and ${u} values.
 - 2) the two targets defined in lines (3) and (6) are searched in the text, 
      keeping the keeping the ${x} and ${u} values found in the trigger string.

If then a target is found, it's replaced by the line(s) defined in the
related replacement(s).

Note that target and replacement may use variables defined in the trigger.
Target may defined new variables. A variable defined in a target can only be
used in the target's own replacement(s).

Using the preceding script, the following text :
   AAA is BBB;
   AAA=BBB+DDD;
   CCC=BBB+BBB;                   <-- (A) Match trigger (1)
   hello
   alpha = beta + beta;           <-- (B) Match trigger (1)
   alpha
   CCC is BBB +CCC                <-- (C) Match target (3) from (A)
   alpha=beta+two*gamma;          <-- (D) Match target (6) from (B)
   the end

will be replaced by :
   AAA is BBB;
   AAA=BBB+DDD;
   CCC=two*BBB;                   <-- Replacement (2) from (A)
   hello
   alpha=two*beta;
   alpha
   CCC is three times BBB         <-- Replacement (4) from (C)
   CCC = three * CCC / two;       <-- Replacement (5) from (C)
   gamma = beta;                  <-- Replacement (7) from (D)
   the end



Now, to make such a replacement, you have to use following bit of script
inside convert.pl :

   # Use brackets to isolate the process from the rest of convert.pl
   {
     # Instantiate a perl object "Process" referenced by $p and
     # working on $code string (don't forget the "my" keyword)
     my $p = new Process($code);

     # Define the trigger string and its replacement
     $p->trigger('${x} = ${u} + ${u} ;');
         $p->replacement('${x}=two*${u};');

         # Define a first target string and two replacements
         $p->target('${y} is ${u} + ${x}');
             $p->replacement('${y} is three times ${u}');
             $p->replacement('${y} = three * ${x} / two;');

         # Define a second target string and one replacement
         $p->target('${x} = ${u} + two * ${z} ;');
             $p->replacement('${z} = ${u};');

     # Do the work then replace $code with the modified string
     $code = $p->exec();
   }
   # The "Process" object is destroyed when this bracket is closed


SOME RULES :

- A replacement process is made of 
     - One and only one trigger definition 
     - As many target definitions as necessary, possibly none
     - As many addAfterLast definitions (see below) as necessary, possibly none

- At least one replacement definition must immediately follow a trigger
  definition.

- At least one replacement definition must immediately follow a target
  definition.

- When defining string arguments to trigger(), target(), replacement() and
  addAfterLast() methods, always use single quote, never double quote (to not
  let perl trying interpolate ${XXX} variables).

- Trigger and target strings always include the start of a line (will avoid
  problems with // comments) 

- Trigger and target strings must be composed of C++ words, C++ operators and
  Process variables each separated from the next with at least one blank
  character (white space or tab). This is mandatory.

- Previous rule doesn't apply on replacement and addAfterLast strings which
  shall have exactly the number and type of white spaces (none if necessary)
  you wish to see in the code after the replacement.

- An element (white space separated) in a trigger or target string may be :
     - a word : consecutive alphanumerical characters (no blank nor punctuation
       or special character)
     - a C++ operator ('*', '->', '=', '(', ')', etc...) (note : all C++
       operators are not currently defined in the Process package)
     - a variable '${name}' where name is an "ordinary" word (ie alphanumerical
       without space).
         Such a variable will match any ordinary word (ie variable name, class
         name or language keyword) in the C++ code text.
     - a list '@{name}',  '@{,name}' or '@{name,}' where name is an "ordinary"
       word (ie alphanumerical without space).
         Such a variable will match almost anything but semicolon in the C++
         code text.

- Any variables or lists may be used in a trigger or target string.

- Any variable or list used in a replacement string should have been already
  used in the trigger or in the target string.

- Exception of the previous string : if a variable (not a list) is used first
  in a target replacement string (this exception is not valid in a trigger
  replacement list), it is replaced by an autogenerated C++ name made from
  the variable name and underscore and a unique (in that process only) number.


MORE ON LISTS :

- @{name} was formerly designed to match a word list inside parenthesis.
  It always matches at least one word. It never matches a string containing
  a semicolon.
      @{name} matches "aaa", or "aaa, bbb" or "aaa.xx("zzz"), bb, ccc"
              but doesn't match "" or "aaa; bbb".

- @{,name} was formerly designed to match a comma separated list on the right
  of a word inside parenthesis.
  It may match nothing. It never matches a string containing a semicolon.
  Whatever string it matches (except null string) begins with a comma.
      @{,name} matches "", or ",aaa" or ',aaa.xx("zzz"), bb, ccc'
              but doesn't match "aaa; bbb" nor "aaa, bbb".

- @{name,} was formerly designed to match a comma separated list on the left
  of a word inside parenthesis.
  It may match nothing. It never matches a string containing a semicolon.
  Whatever string it matches (except null string) is terminated with a comma.
      @{name,} matches "", or "aaa," or 'aaa.xx("zzz"), bb, ccc,'
              but doesn't match "aaa; bbb" nor "aaa, bbb".

In a trigger or target definition, following structure
        ...  some_text ( @{left_list,} ${some_variable} @{right_list,} ) ...
 may be useful to match  "some_text(aaa, bbb, ccc, the_variable, ddd, eee)"
                     or  "some_text(aaa, bbb, ccc, the_variable)"
                     or  "some_text(the_variable, ddd, eee)"
                     or  "some_text(the_variable)"


MORE ON AUTOGENERATED IDENTIFIERS :

  If this code
      aaa = new Widget(parent);

  shall be replaced with
      aaa = new Widget;
      layout->addWidget(aaa);

  following commands may be used (assuming that ${layout} is defined
  in trigger string) :
      $p->target('${child} = new Widget ( ${parent) } ;');
      $p->replacement('${child} = new Widget ;');
      $p->replacement('${layout} -> addWidget ( ${child) } ;');

  
  now if initial code is only
      new Widget(parent);

  following commands may be used (assuming that ${layout} is defined
  in trigger string) :
      $p->target('new Widget ( ${parent) } ;');
      $p->replacement('Widget *${child} = new Widget ;');
      $p->replacement('${layout} -> addWidget ( ${child) } ;');

  if ${child} is not defined in trigger nor in target, it will be autogenerated
  and we get the following new C++ code :
      Widget *child_34 = new Widget;
      layout->addWidget(child_34);


ADDAFTERLAST COMMAND :

  This command may be used to insert a line, defined as a trigger replacement
  (ie using only variables coming from the trigger), after the last replacement
  from several targets.

  Targets to be used are identified by character string (some name) passed as 
  a second argument (optional) of the target() method :
     $p->target(' ... target string ...', "identifier");

  The same identifier should be pass as the first argument (mandatory) of the 
  addAfterLast() method.
  The second arg. of the method is the replacement string.

  Example :
    $p-> addAfterLast"identifier", ' replacement string ');


WARNINGS AND ERRORS :

  A warning is issued on STDERR each time an identifier is autogenerated.
  This may help to detect typo (different variable names in target and in
  replacement) producing erroneous autogenerated identifier.

  If an error occurs, look at the beginning of the message, where the
  problem encountered should be wrote and at the end of the message where the
  line number in convert.pl should be shown.

  Note that lot of errors are probably undetected and may cause weird
  behaviours.

=cut

#
###    The code  begins here  ###
#


{ package Element;
use strict;
use Carp;

# An element object is a word in the string associated to a trigger or a target

# Creation of a new Element object. No arg.
sub new {
    my $class = shift;
    my $self  = {};
    $self->{STRING}   = undef;
    $self->{TYPE} = undef;
    $self->{NAME} = undef;
    bless ($self, $class);
    $self->string(@_) if @_;
    return $self;
} 

# Assign a string to a newly created element
sub string {
    my $self = shift;
    if (@_) {
        $self->{STRING} = shift;
        if ($self->{STRING} =~ m/^\w+$/) {
            $self->{TYPE} = "text";
        } elsif (   $self->{STRING} eq '*'
                 || $self->{STRING} eq ';'
                 || $self->{STRING} eq '('
                 || $self->{STRING} eq ')'
                 || $self->{STRING} eq '->'
                 || $self->{STRING} eq '='
                 || $self->{STRING} eq '+'
                 || $self->{STRING} eq '-'
                 || $self->{STRING} eq '.'
                 || $self->{STRING} eq ','
                 || $self->{STRING} eq '/'
                 || $self->{STRING} eq '['
                 || $self->{STRING} eq ']'
                 || $self->{STRING} eq '{'
                 || $self->{STRING} eq '}'
                 || $self->{STRING} eq '?'
                 || $self->{STRING} eq ':'
                 || $self->{STRING} eq '::'
                 || $self->{STRING} eq '<'
                 || $self->{STRING} eq '>'
                 || $self->{STRING} eq '=='
                 || $self->{STRING} eq '!='
                 || $self->{STRING} eq '!'
                 || $self->{STRING} eq '++'
                 || $self->{STRING} eq '--'
                 || $self->{STRING} eq '<<'
                 || $self->{STRING} eq '>>'
                 || $self->{STRING} eq '^'
                 || $self->{STRING} eq '~'
                 || $self->{STRING} eq '&'
                 || $self->{STRING} eq '&&'
                 || $self->{STRING} eq '|'
                 || $self->{STRING} eq '||') {
            $self->{TYPE} = "operator";
        } elsif ($self->{STRING} =~ m/^\${(\w+)}$/) {
            $self->{TYPE} = "variable";
            $self->{NAME} = $1;
        } elsif ($self->{STRING} =~ m/^\@{(\w+),}$/) {
            $self->{TYPE} = "list";
            $self->{SUBTYPE} = "left";
            $self->{NAME} = $1;
        } elsif ($self->{STRING} =~ m/^\@{,(\w+)}$/) {
            $self->{TYPE} = "list";
            $self->{SUBTYPE} = "right";
            $self->{NAME} = $1;
        } elsif ($self->{STRING} =~ m/^\@{(\w+)}$/) {
            $self->{TYPE} = "list";
            $self->{SUBTYPE} = "center";
            $self->{NAME} = $1;
        } else {
            $self->{TYPE} = "???";
            $self->{NAME} = "???";
            print stderr "***   ERROR : String = \"", $self->{STRING}, "\"\n";
            confess "Unexpected word in a trigger or target definition ";
        }
    }
    return $self->{STRING};
}

# Return the type of an element
# ("text", "variable", "operator", "list", "llist" or "rlist")
sub type {
    my $self = shift;

    if ($self->{TYPE} eq "list") {
        if ($self->{SUBTYPE} eq "right") {
            return "rlist";
        }
        if ($self->{SUBTYPE} eq "left") {
            return "llist";
        }
        if ($self->{SUBTYPE} eq "center") {
            return "list";
        }
        confess "Internal error : \$self->{TYPE} = \"" . $self->{TYPE} .
                "\"   \$self->{SUBTYPE} = \"" . $self->{SUBTYPE} . "\"   ";
    }

    return $self->{TYPE};
}

sub variableName {
    my $self = shift;
    return $self->{NAME};
}

# Return the separator to use in RE between element and "last" element
# passed as an arg.
sub separator {
    my $self = shift;
    my $last = shift;

    return "" if !$last->type();

    # Which kind of spacing to use between two elements in a RE
    my $l = $last->{TYPE};
    my $r = $self->{TYPE};
    if (($l eq "operator") || ($r eq "operator")) { return '\s*'; }
    if (($l eq "list") || ($r eq "list")) { return '\s*'; }
    return '\s+';
}

1;
} # End of package Element





{ package Substitution;
use strict;
use Carp;

# A substitution object is a trigger or a target
# It includes a SRC string (the trigger or target string) and a list
# of DST strings (the replacement strings)

sub new {
    my $class = shift;
    my $self  = {};
    bless ($self, $class);
    return $self;
}

sub src {
    my $self = shift;
    my $str = shift;

    $self->{SRC} = $str;
    $self->{DST} = [];
    $self->makeRegExp();
}

sub lastId {
    my $self = shift;
    my $str = shift;

    $self->{LAST} = $str;
}

sub dst {
    my $self = shift;
    my $str = shift;

    # Here we should test if var names used in $str are defined
    push @{$self->{DST}}, $str;
}

# Return alist of all the variable names
sub getVarNameList {
    my $self = shift;
    return keys %{$self->{VAR}};
}

# Return the number of the positional parameter associated to a variable
# (name in arg.) in the RE created from SRC
sub getVarPos {
    my $self = shift;
    my $varName = shift;
    return $self->{VAR}{$varName}{POS};
}

# Return alist of all the list names
sub getListNameList {
    my $self = shift;
    return keys %{$self->{LIS}};
}

# Return the number of the positional parameter associated to a list
# (name in arg.) in the RE created from SRC
sub getListPos {
    my $self = shift;
    my $varName = shift;
    return $self->{LIS}{$varName}{POS};
}

# Return the type of a list (name in arg.)
sub getListType {
    my $self = shift;
    my $varName = shift;
    return $self->{LIS}{$varName}{TYPE};
}

# Generate RE from source string.
# Source string is internal SRC string if no arg. given, else
# arg. is use as source string.
sub makeRegExp {
    my $self = shift;

    undef $self->{RE};
    undef %{$self->{VAR}};
    undef %{$self->{LIS}};
    undef %{$self->{POS}};

    my $src = ($#_ == -1) ? $self->{SRC} : shift;
    my @x = split ' ', $src;

    my $last = Element->new();

    # Always match the beginning of a line (and get the indentation)
    #  ==> This protects against the risk of working on a // comment
    #      (but doesn't protect against /* */ comment :(
    #       nevertheless, such a comment is probably less dangerous
    #       as inserted lines will stay inside the comment )
    my $r = "\n([\t ]*)";

    my $parenthCount = 1;   # First parenthesis is from indentation group

    loop:
    foreach my $zz (@x) {
        my $el = Element->new($zz);

        #print "elem : >", $el->string(), "<";
        #print "    type=", $el->type();

        $r .= $el->separator($last);

        if ($el->type() eq "variable") {
            if (exists $self->{VAR}{$el->variableName()}) {
                $r .= '\\' . $self->{VAR}{$el->variableName()}{POS};
            } else {
                $self->{VAR}{$el->variableName()}{POS} = ++$parenthCount;
                $self->{POS}{$parenthCount}{NAME} = $el->variableName();
                $self->{POS}{$parenthCount}{TYPE} = '$';
                $r .= '(\w+)';
            }
        } elsif ($el->type() eq "list") {
            $self->{LIS}{$el->variableName()}{POS} = ++$parenthCount;
            $self->{LIS}{$el->variableName()}{TYPE} = '@';
            $self->{POS}{$parenthCount}{NAME} = $el->variableName();
            $self->{POS}{$parenthCount}{TYPE} = '@';
            $r .= '((\s*[^,;]+\s*,)*(\s*[^,;]+\s*){1})';
            $parenthCount += 2;
        } elsif ($el->type() eq "llist") {
            $self->{LIS}{$el->variableName()}{POS} = ++$parenthCount;
            $self->{LIS}{$el->variableName()}{TYPE} = '@,';
            $self->{POS}{$parenthCount}{NAME} = $el->variableName();
            $self->{POS}{$parenthCount}{TYPE} = '@,';
            $r .= '([^;,]*,)*';
        } elsif ($el->type() eq "rlist") {
            $self->{LIS}{$el->variableName()}{POS} = ++$parenthCount;
            $self->{LIS}{$el->variableName()}{TYPE} = ',@';
            $self->{POS}{$parenthCount}{NAME} = $el->variableName();
            $self->{POS}{$parenthCount}{TYPE} = ',@';
            $r .= '(,[^;,]*)*';
        } elsif ($el->type() eq "text") {
            $r .= $el->string();
        } elsif ($el->type() eq "operator") {
            my $s = $el->string();
            $s =~ s/([\*\(\)\.\\\|\[\]\{\}\+\?\^]{1})/\\$1/gs; 
            $r .= $s;
        } else {
            $r .= '???';
        }

        $last = $el;
    }

    $self->{RE} = $r;
}

# Return how many replacements are defined inside the substitution object
sub replacementCount {
    my $self = shift;
    return scalar @{$self->{DST}};
}

# Only for debug
sub dump {
    my $self = shift;
    print STDERR "SRC : ", $self->{SRC}, "\n";
    print STDERR "RE : ", $self->{RE}, "\n";
    foreach my $var (keys %{$self->{VAR}}) {
        print STDERR "   var : ", $self->{VAR}{$var}{POS}, " : ", $var, "\n";
    }
    foreach my $lis (keys %{$self->{LIS}}) {
        print STDERR "   lis ", $self->{LIS}{$lis}{TYPE},
                     " : ", $self->{LIS}{$lis}{POS}, " : ", $lis, "\n";
    }
    foreach my $pos (keys %{$self->{POS}}) {
        print STDERR "   pos : ", $self->{POS}{$pos}{TYPE}, " ",
                           $self->{POS}{$pos}{NAME}, "\n";
    }
    foreach my $dst (@{$self->{DST}}) {
        print STDERR "DST : ", $dst, "\n";
    }
}


1;
} # End of package Substitution



{ package Process;
use strict;
use Carp;

# Process object is used to handle the trigger, targets and associated
# replacements then to execute the replacements.

sub new {
    my $class = shift;
    my $self  = {};
    $self->{CODE}   = shift;
    $self->{PHASE} = 0;
    $self->{LAST} = {};
    bless ($self, $class);
    return $self;
}

sub trigger {
    my $self = shift;
    my $str = shift;

    confess "Trigger already defined " if $self->{PHASE} != 0;

    $self->{TRIG} = Substitution->new();
    $self->{TRIG}->src($str);
    $self->{PHASE} = 1;
    $self->{TARGET} = [];
}

sub replacement {
    my $self = shift;
    my $str = shift;

    confess "Replacement defined without trigger " if $self->{PHASE} == 0;

    if ($self->{PHASE} == 1) {
        $self->{TRIG}->dst($str);
    } elsif ($self->{PHASE} == 2) {
        $self->{LASTTARGET}->dst($str);
        $self->{PHASE} = 3;
    } else {  # PHASE == 3
        $self->{LASTTARGET}->dst($str);
    }
}

sub target {
    my $self = shift;
    my $str = shift;
    my $lastId = shift;

    confess "Target defined without trigger " if $self->{PHASE} == 0;

    if ($self->{PHASE} == 1) {
        confess "Trigger has no replacement "
                                if $self->{TRIG}->replacementCount() == 0;
    }

    confess "Target has no replacement " if $self->{PHASE} == 2;

    $self->{LASTTARGET} = Substitution->new();
    $self->{LASTTARGET}->src($str);
    if (defined $lastId) {
        $self->{LASTTARGET}->lastId($lastId);
    }
    push @{$self->{TARGET}}, $self->{LASTTARGET};
    $self->{PHASE} = 2;
}

sub addAfterLast {
    my $self = shift;
    my $id = shift;
    my $str = shift;

    confess "AddAfterLast defined without trigger " if $self->{PHASE} == 0;

    push @{$self->{LAST}{$id}}, $str;
}

# Only for debug
sub dump {
    my $self = shift;

    $self->{TRIG}->dump();
    print STDERR "====================================\n";
    foreach my $target (@{$self->{TARGET}}) {
        $target->dump();
        print STDERR "------------------------\n";
   }
}

# For debug only
sub dumpVar {
    my $self = shift;
    my %memo = @_;
    print STDERR "  SOURCE : \"", $memo{SRC}, "\"\n";
    print STDERR "  RE : \"", $memo{RE}, "\"\n";
    print STDERR "  LINE : \"", $memo{ORIG}, "\"\n";
    for my $v (keys %{$memo{VAR}}) {
        print STDERR "    \${$v} : \"", $memo{VAR}{$v}, "\"\n";
    }
    for my $v (keys %{$memo{LIS}}) {
        print STDERR "    \@{$v} : \"", $memo{LIS}{$v}{VALUE}, "\"\n";
    }
}

sub exec {
    my $self = shift;
    my $ruid = 0;
    my $trig = $self->{TRIG}->{RE};

    my %memo = ();
    my %finalSubstitution = ();

    # Make a list of triggers and related variables
    # substitute each trigger with a unique keyword
    while  ($self->{CODE} =~ s/$trig/XXXXYYYYZZZZ_TRIGGER_$ruid/s) {
        # Remember original string for debug
        $memo{"XXXXYYYYZZZZ_TRIGGER_$ruid"}{SRC} = $self->{TRIG}->{SRC};
        $memo{"XXXXYYYYZZZZ_TRIGGER_$ruid"}{RE} = $trig;
        $memo{"XXXXYYYYZZZZ_TRIGGER_$ruid"}{ORIG} = $&;

        # Remember indentation
        $finalSubstitution{"XXXXYYYYZZZZ_TRIGGER_$ruid"}{INDENT} = $1;

        # Remember contents of variables
        foreach my $var ($self->{TRIG}->getVarNameList()) {
            no strict 'refs';
            $memo{"XXXXYYYYZZZZ_TRIGGER_$ruid"}{VAR}{$var}
                                    = ${$self->{TRIG}->getVarPos($var)};
        }

        # Remember contents of lists
        foreach my $list ($self->{TRIG}->getListNameList()) {
            $memo{"XXXXYYYYZZZZ_TRIGGER_$ruid"}{LIS}{$list}{TYPE}
                                    = $self->{TRIG}->getListType($list);
            no strict 'refs';
            $memo{"XXXXYYYYZZZZ_TRIGGER_$ruid"}{LIS}{$list}{VALUE}
                                    = ${$self->{TRIG}->getListPos($list)};
        }

        $ruid++;
    }

    # Now, prepare then do the substitutions
    foreach my $kw (keys %memo) {

        my %memoLast = ();  # Where to store data about "AddAfterLast"

        # get the indentation
        # $self->{CODE} =~ m/\n([\t ]*)$kw/s;
        # my $indent = $1;
        my $indent = $finalSubstitution{$kw}{INDENT};

        # concatenate the trigger replacement strings
        my $dest = "";
        foreach my $d (@{$self->{TRIG}->{DST}}) {
            $dest .= "\n" . $indent . $d;
        }

        # substitute variable names with values
        foreach my $v (keys %{$memo{$kw}{VAR}}) {
            my $r = $memo{$kw}{VAR}{$v};
            $dest =~ s/\${$v}/$r/sg;
        }

        # substitute list names with values
        foreach my $l (keys %{$memo{$kw}{LIS}}) {
            my $r = $memo{$kw}{LIS}{$l}{VALUE};
            my $ty = $memo{$kw}{LIS}{$l}{TYPE};
            if ($ty eq '@') {
                $dest =~ s/\@{$l}/$r/sg;
            } elsif ($ty eq '@,') {
                $dest =~ s/\@{$l,}/$r/sg;
            } else {
                $dest =~ s/\@{,$l}/$r/sg;
            }
        }

        # All variables and lists must be replaced
        if ($dest =~ m/\${(\w+)}/) {
            warn "*** ERROR *** ";
            print STDERR "Variable \${$1} has no value in trigger replacement string";
            print STDERR "REPLACEMENT : $dest";
            print STDERR "TRIGGER VAR DUMP :\n";
            $self->dumpVar(%{$memo{$kw}});
            confess " ";

        }
        if ($dest =~ m/\@{(,?\w+,?)}/) {
            warn "*** ERROR *** ";
            print STDERR "list \@{$1} has no value in trigger replacement string";
            print STDERR "REPLACEMENT : $dest";
            print STDERR "TRIGGER VAR DUMP :\n";
            $self->dumpVar(%{$memo{$kw}});
            confess " ";
        }

        # do the trigger substitution
        # $self->{CODE} =~ s/\n$indent$kw/$dest/s;
        $finalSubstitution{"$kw"}{REPL} = $dest;

        # Then, process the related targets
        foreach my $t (@{$self->{TARGET}}) {
            my $src = $t->{SRC};

            my $lastId = defined $t->{LAST} ? "-" . $t->{LAST} : "";

            # Substitute known variable and list names in target string
            foreach my $v (keys %{$memo{$kw}{VAR}}) {
                my $r = $memo{$kw}{VAR}{$v};
                $src =~ s/\${$v}/$r/sg;
            }
            foreach my $l (keys %{$memo{$kw}{LIS}}) {
                my $r = $memo{$kw}{LIS}{$l}{VALUE};
                $src =~ s/\@{,?$l,?}/$r/sg;
            }

            # Compute a new target RE after variable substitutions
            $t->makeRegExp($src);
            my $target = $t->{RE};

            # Make a list of targets and related variables
            # substitute each target with a unique keyword
            my %targetMemo = ();
            while  ($self->{CODE} =~ s/$target/XXXXYYYYZZZZ_TARGET_$ruid$lastId/s) {

                # Remember original string for debug
                $targetMemo{"XXXXYYYYZZZZ_TARGET_$ruid$lastId"}{SRC} = $t->{SRC};
                $targetMemo{"XXXXYYYYZZZZ_TARGET_$ruid$lastId"}{RE} = $target;
                $targetMemo{"XXXXYYYYZZZZ_TARGET_$ruid$lastId"}{ORIG} = $&;

                # Remember indentation
                $finalSubstitution{"XXXXYYYYZZZZ_TARGET_$ruid$lastId"}{INDENT} = $1;

                # Remember contents of variables
                foreach my $var ($t->getVarNameList()) {
                    no strict 'refs';
                    $targetMemo{"XXXXYYYYZZZZ_TARGET_$ruid$lastId"}{VAR}{$var}
                                                  = ${$t->getVarPos($var)};
                }

                # Remember contents of lists
                foreach my $list ($t->getListNameList()) {
                    $targetMemo{"XXXXYYYYZZZZ_TARGET_$ruid$lastId"}{LIS}{$list}{TYPE}
                                                  = $t->getListType($list);
                    no strict 'refs';
                    $targetMemo{"XXXXYYYYZZZZ_TARGET_$ruid$lastId"}{LIS}{$list}{VALUE}
                                                  = ${$t->getListPos($list)};
                }

                $ruid++;
            }

            # Now, prepare then do the final target substitutions
            foreach my $tkw (keys %targetMemo) {

                # Get indentation
                # $self->{CODE} =~ m/\n([\t ]*)$tkw/s;
                # my $indent = $1;
                my $indent = $finalSubstitution{$tkw}{INDENT};

                # concatenate the target replacement strings
                my $dest = "";
                foreach my $d (@{$t->{DST}}) {
                    $dest .= "\n" . $indent . $d;
                }

                # substitute variable names with values from trigger
                foreach my $v (keys %{$memo{$kw}{VAR}}) {
                    my $r = $memo{$kw}{VAR}{$v};
                    $dest =~ s/\${$v}/$r/sg;
                }

                # substitute variable names with values from target
                foreach my $v (keys %{$targetMemo{$tkw}{VAR}}) {
                    my $r = $targetMemo{$tkw}{VAR}{$v};
                    $dest =~ s/\${$v}/$r/sg;
                }

                # substitute list names with values from trigger
                foreach my $l (keys %{$memo{$kw}{LIS}}) {
                    my $r = $memo{$kw}{LIS}{$l}{VALUE};
                    my $ty = $memo{$kw}{LIS}{$l}{TYPE};
                    if ($ty eq '@') {
                        $dest =~ s/\@{$l}/$r/sg;
                    } elsif ($ty eq '@,') {
                        $dest =~ s/\@{$l,}/$r/sg;
                    } elsif ($ty eq ',@') {
                        $dest =~ s/\@{,$l}/$r/sg;
                    } else {
                        confess "Internal Error \$r=\"$r\" \$ty=\"$ty\" ";
                    }
                }

                # substitute list names with values from target
                foreach my $l (keys %{$targetMemo{$tkw}{LIS}}) {
                    my $r = $targetMemo{$tkw}{LIS}{$l}{VALUE};
                    my $ty = $targetMemo{$tkw}{LIS}{$l}{TYPE};
                    if ($ty eq '@') {
                        $dest =~ s/\@{$l}/$r/sg;
                    } elsif ($ty eq '@,') {
                        $dest =~ s/\@{$l,}/$r/sg;
                    } elsif ($ty eq ',@') {
                        $dest =~ s/\@{,$l}/$r/sg;
                    } else {
                        confess "Internal Error \$r=\"$r\" \$ty=\"$ty\" ";
                    }
                }

                # Each list must be replaced
                if ($dest =~ m/\@{(,?\w+,?)}/s) {
                    warn "*** ERROR *** ";
                    print STDERR "List \@{$1} has no value in target replacement string\n";
                    print STDERR "REPLACEMENT : $dest\n";
                    # $self->dump();
                    print STDERR "TRIGGER VAR DUMP :\n";
                    $self->dumpVar(%{$memo{$kw}});
                    print STDERR "TARGET VAR DUMP :\n";
                    $self->dumpVar(%{$targetMemo{$tkw}});
                    confess "";
                }

                # An unreplaced variable is assumed to have to be
                # automatically generated - A warning is issued on stderr.
                while ($dest =~ m/\${(\w+)}/s) {
                    my $varName = $1;
                    my $autoName = "${varName}_$ruid";
                    $ruid++;
                    $dest =~ s/\${$varName}/$autoName/sg;
                    print STDERR "*** WARNING ***\n";
                    print STDERR "  \${$varName} replaced by auto generated name \"$autoName\"\n";
                    print STDERR "REPLACEMENT : $dest\n";
                }

                # do the target substitution
                # $self->{CODE} =~ s/\n$indent$tkw/$dest/s;
                $finalSubstitution{"$tkw"}{REPL} = $dest;
            }

        }

        # Now append "addAfterLast" strings if any

        LASTID :
        foreach my $id (keys %{$self->{LAST}}) {

            # Computed $lastLines value is not useful (it's the last find
            # out, but not the last in the source text.
            # Look for the place where insert $dst again now.
            my $place = "";
            while ($self->{CODE} =~ m/(XXXXYYYYZZZZ_TARGET_\d+-$id)/sg) {
                $place = $1;
            }
            next LASTID if $place eq "";

            my $indent = $finalSubstitution{$place}{INDENT};

            my $dst = "";
            foreach my $s (@{$self->{LAST}{$id}}) {
                $dst .= "\n$indent" . $s;
            }

            # substitute variable names with values
            foreach my $v (keys %{$memo{$kw}{VAR}}) {
                my $r = $memo{$kw}{VAR}{$v};
                $dst =~ s/\${$v}/$r/sg;
            }

            # substitute list names with values
            foreach my $l (keys %{$memo{$kw}{LIS}}) {
                my $r = $memo{$kw}{LIS}{$l}{VALUE};
                $dst =~ s/\@{,?$l,?}/$r/sg;
            }

            # All variables and lists must be replaced
            if ($dest =~ m/\${\w+}/) {
                warn "*** ERROR *** ";
                print STDERR "Variable \${$1} has no value in \"add after last\" string (lastId = $id)";
                print STDERR "LINES TO ADD : $dst";
                print STDERR "TRIGGER VAR DUMP :\n";
                $self->dumpVar(%{$memo{$kw}});
                confess " ";

            }
            if ($dest =~ m/\@{,?\w+,?}/) {
                warn "*** ERROR *** ";
                print STDERR "list \@{$1} has no value in \"add after last\" string (lastId = $id)";
                print STDERR "LINES TO ADD : $dst";
                print STDERR "TRIGGER VAR DUMP :\n";
                $self->dumpVar(%{$memo{$kw}});
                confess " ";
            }

            # Add the lines
            $self->{CODE} =~ s/$place/$place$dst/s;
        }

        # Do the final substitutions
        # (delayed to now for not to have already replaced strings found
        #  as targets to be replaced)
        foreach my $xxx_id (keys %finalSubstitution) {
            $self->{CODE} =~ s/$xxx_id/$finalSubstitution{$xxx_id}{REPL}/s;
        }
    }

    return $self->{CODE};
}

1;
} # End of package Process


