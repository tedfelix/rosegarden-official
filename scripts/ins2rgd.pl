#!/usr/bin/perl

# ins2rgd.pl
# Takes a Cakewalk/Sonar .ins instrument definition file on stdin
# emits an (uncompressed) .rgd, which the user then compresses and places
# in the appropriate directory (usually /usr/share/apps/rosegarden/library).
#
# (C)2005 Scott Doty <scott@sonic.net>
# Permission to copy granted in accordance with the Perl Artistic License.
#
# http://ponzo.net/Rosegarden/
#
# BUGS: This could still use some work -- there are hard-coded values that
#       could be configured...
# 
# Modified by Pedro Lopez-Cabanillas <plcl@users.sf.net> (Jul/2005)
# -> include Controllers and KeyMappings.
#
# Usage:
#	ins2rgd.pl original.ins | gzip > converted.rgd

my $devicename = undef;
my $librarian = "Created using ins2rgd.pl";
my $librarian_email = "rosegarden-devel\@lists.sourceforge.net";

use constant IN_PATCH => 1;	# .Patch Names section
use constant IN_INSTR => 2;	# .Instrument Definitions section
use constant IN_NOTES => 3;	# .Note Names section
use constant IN_CTRLS => 4;	# .Controller Names section

my $flag_in = undef;	
my $current = undef;		# current bank
my $banks = {};			# the in-memory banks database
my $keymaps = {};
my $controllers = {};
my $banknumof = {};
my $keybased = {};
my $mappedkeys = {};
my $bankselmethod = 0;

# sussed from txt2rgd.py
%converter_table = (
  '&'	=>	'&amp;' ,
  '<'	=>	'&lt;'  ,
  '>'	=>	'&gt;'	,
  '"'	=>	'&quot;',
  "'"	=>	'&apos;'
);

LINE:
while(<>)
  {
s/\s+$//; # might be cr/lf too
  next if (/^\#/);
  next if (/^$/);
  next if (/^\;/);

  if(/^\.Patch Names/i)
    {
    $flag_in=IN_PATCH;
    next LINE;
    }

  if(/^\.Instrument Definitions/i)
    {
    $flag_in=IN_INSTR;
    next LINE;
    }

  if(/^\.Note Names/i)
    {
    $flag_in=IN_NOTES;
    next LINE;
    }

  if(/^\.Controller Names/i)
    {
    $flag_in=IN_CTRLS;
    next LINE;
    }

  if(/^\./i)
    {
    $flag_in=undef;
    next LINE;
    }

  if (defined($flag_in)) 
    {

    if($flag_in == IN_PATCH)
      {
      if(m#^\[(.+)\]$#)
        {
        $current = doquote($1);
        $banks->{$current} = {};
        next LINE;
        }
      if(m#^(\d+)\=(.+)$#)
        {
        my $voicenum = $1;
        my $voicename = doquote($2);
      
        $banks->{$current}->{$voicenum} = $voicename;
        }
      }

      if($flag_in == IN_INSTR)
        {
        if(m#^\[(.+)\]$#)
          {
	  unless(defined($devicename))
            {
            $devicename = doquote($1);
            }
	  next LINE;
          }
        if(m#^BankSelMethod\=(\d+)$#)
          {
	  $bankselmethod=$1;
	  next LINE;
	  }
        if(m#Patch\[(\d+)\]=(.+)$#)
          {
          my $banknum = $1;
          my $bankname = doquote($2);
      
          $banknumof{$bankname}=$banknum;
  	  next LINE;
          }
        if(m#Key\[(\d+)\,(\d+)\]=(.+)$#)
          {
          my $keymapname = doquote($3);
	  $mappedkeys->{$1}->{$2} = $keymapname;
          }
      }

    if($flag_in == IN_NOTES)
      {
      if(m#^\[(.+)\]$#)
        {
        $current = doquote($1);
        $keymaps->{$current} = {};
        next LINE;
        }
      if(m#^BasedOn\=(.+)$#)
        {
	my $basemap = doquote($1);
	$keybased{$current} = $basemap;
	}
      if(m#^(\d+)\=(.+)$#)
        {
        my $keynum = $1;
        my $keyname = $2;
      
        $keymaps->{$current}->{$keynum} = doquote($keyname);
        }
      }

    if($flag_in == IN_CTRLS)
      {
      if(m#^(\d+)\=(.+)$#)
        {
	my $ctl = $1;
        my $controlname = $2;
	unless($ctl == 1 || $ctl == 7 || $ctl == 10 || $ctl == 11 || $ctl == 64 || $ctl == 91 || $ctl == 93)
	  {
          $controllers{$ctl} = doquote($controlname);
	  }
        }
      }
    }
  }

# end of .ins file -- write .rgd xml

# header
print '<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE rosegarden-data>
<rosegarden-data>
<studio thrufilter="0" recordfilter="0">
<device id="0" name="',$devicename,'" direction="play" variation="" type="midi">
	<librarian name="',$librarian,'" email="',$librarian_email,'"/>
	<metronome instrument="0" msb="0" lsb="0" program="0" pitch="37" />
';

# generate list of banks sorted by bank number
@banks = sort { $banknumof{$a} <=> $banknumof{$b} } ( keys %banknumof);

for my $bank (@banks)
  {
  my $banknum = $banknumof{$bank};
  my $msb;
  my $lsb;
  my $percussion;

  if ($bankselmethod == 1)
    {
    $msb = $banknum;
    $lsb = 0;
    }
    else
    {
    $msb = int( $banknum / 128 );
    $lsb = $banknum - (128*$msb);
    }
  
  if ($bank =~ /drum/i)
    {
    $percussion='true';
    }
    else
    {
    $percussion='false';
    }
  
  print qq{\t<bank name="$bank" percussion="$percussion" msb="$msb" lsb="$lsb">};
  print "\n";

  @voicenums = sort
      { $a <=> $b }
      (keys %{ $banks->{$bank} } );

  for $voicenum (@voicenums)
    {
    $voicename = $banks->{$bank}->{$voicenum};
    print qq{\t\t<program id="$voicenum" name="$voicename"};
    if ($percussion eq 'true') 
      {
      if (defined($mappedkeys->{$banknum}->{$voicenum}))
        {
        print qq{ keymapping="$mappedkeys->{$banknum}->{$voicenum}"};
	}
      } 
    print "/>\n";
    }

  print "\t</bank>\n\n";

  }

#standard controllers
print ' 
	<controls>
		<control name="Pan" type="controller" description="&lt;none&gt;" min="0" max="127" default="64" controllervalue="10" colourindex="2" ipbposition="0"/>
		<control name="Chorus" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="93" colourindex="3" ipbposition="1"/>
		<control name="Volume" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="7" colourindex="1" ipbposition="2"/>
		<control name="Reverb" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="91" colourindex="3" ipbposition="3"/>
		<control name="Sustain" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="64" colourindex="4" ipbposition="4"/>
		<control name="Expression" type="controller" description="&lt;none&gt;" min="0" max="127" default="100" controllervalue="11" colourindex="2" ipbposition="5"/>
		<control name="Modulation" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="1" colourindex="4" ipbposition="-1"/>
		<control name="PitchBend" type="pitchbend" description="&lt;none&gt;" min="0" max="16383" default="8192" controllervalue="1" colourindex="4" ipbposition="-1"/>
';

# some additional controllers
for $controlnum (keys %controllers)
 {
 $controlname = $controllers{$controlnum};
 print qq{\t\t<control name="$controlname" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="$controlnum" colourindex="4" ipbposition="-1"/>\n};
 }
print "\t</controls>\n\n";

# key mappings

for $keymap (keys %{$keymaps})
  {
  my $base = $keybased{$keymap};

  print qq{\t<keymapping name="$keymap">\n};  

  if(defined($base))
    {
    for $key (keys %{$keymaps->{$base}})
      {
      unless(defined($keymaps->{$keymap}->{$key}))
        {
	$keymaps->{$keymap}->{$key} = $keymaps->{$base}->{$key};
	}
      }
    }

  @keyes = sort { $a <=> $b } (keys %{ $keymaps->{$keymap} } );

  for $key (@keyes)
    {
    $keyname = $keymaps->{$keymap}->{$key};
    print qq{\t\t<key number="$key" name="$keyname"/>\n};
    }

  print "\t</keymapping>\n\n";
  }

# end of device definition
print '</device>
</studio>
</rosegarden-data>
';

exit 0;

  
sub doquote
{
my $n = shift;

my $retval;

my @chars = split(//,$n);

for my $char (@chars)
  {
  if(exists($converter_table{$char}))
    {
    $retval .= $converter_table{$char};
    }
    else
    {
    $retval .= $char;
    }
  }

$retval;
}
