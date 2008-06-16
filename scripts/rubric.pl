#!/usr/bin/perl

### Updates a source file from the "old" to "new" rubric style (from
### naming individual core developers to crediting the Rosegarden team
### generally).
  
my $code = join '', <>; # read the entire file into a single string

$code =~ s/(\s*
)?    This program is Copyright 2000-200.
        Guillaume Laurent   <[^>]*>,
        Chris Cannam        <[^>]*>,
        Richard Bown        <[^>]*>
/
    Copyright 2000-2008 the Rosegarden development team.
/;

$code =~ s/(\s*
)?  This program is Copyright 2000-2008
  Guillaume Laurent   <[^>]*>,
  Chris Cannam        <[^>]*>,
  Richard Bown        <[^>]*>
/
  Copyright 2000-2008 the Rosegarden development team.
/;

$code =~ s/    The moral right of the authors to claim authorship of this work
    has been asserted.
\s*
//;

$code =~ s/  The moral right of the authors to claim authorship of this work
  has been asserted.
\s*
//;

$code =~ s/    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
\s*
//;

$code =~ /AUTHORS/ or
  $code =~ s/    Copyright 2000-2008 the Rosegarden development team./    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details./;

print $code;
