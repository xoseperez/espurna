#!/usr/bin/perl

use POSIX;
use Getopt::Long;

my $exp = '';
my $all = 0;

&GetOptions ('e=s' => \$exp,
             'a' => \$all);
my $csv = 'https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv';

my $re = qr/.*$exp.*/;

local $/;  # enable localized slurp mode

open(my $fh, "curl $csv |") or die "failed: $csv";
# open(my $html, ">", "TZ.html") or die "failed to open TZ.html";
open(my $ch, ">", "../espurna/TZ.h") or die "failed to open ../espurna/TZ.h";
open(my $js, ">", "TZ.js") or die "failed to open TZ.js";

printf $js ("// This file is generate with TZupdate.pl in espurna/code/html\n");
printf $js ("// It uses the content of %s\n", $csv);
printf $js ("// It work with TZ.h (also generated)\n\n");
printf $js (" function loadAllTimeZones() {
     var cit = \$(\"select[name='ntpOffset']\");\n");

#printf $html ("
#   <select id=\"timeZone\" multiple>\n");
printf $ch ("// This file is generate with TZupdate.pl in espurna/code/html\n");
printf $ch ("// It uses the content of %s\n", $csv);
printf $ch ("// It work with TZ.js (also generated)\n\n");

printf $ch (
"typedef struct { 
    char * timeZoneName;
    int16_t  timeZoneOffset;    // offset from GMT 0 in minutes 
    char * timeZoneDSTName;     // NULL if disable
    int16_t  timeZoneDSTOffset; // offset from GMT 0 in minutes 
    uint8_t dstStartMonth;      // start of Summer time if enabled  Month 1 - 12, 0 disabled dst
    uint8_t dstStartWeek;       // start of Summer time if enabled Week 1 - 5: (5 means last)
    uint8_t dstStartDay;        // start of Summer time if enabled Day 1- 7  (1- Sun)
    int16_t dstStartMin;       // start of Summer time if enabled in minutes
    uint8_t dstEndMonth;        // end of Summer time if enabled  Month 1 - 12
    uint8_t dstEndWeek;         // end of Summer time if enabled Week 1 - 5: (5 means last)
    uint8_t dstEndDay;          // end of Summer time if enabled Day 1-7  (1- Sun)
    int16_t dstEndMin;         // end of Summer time if enabled in minutes
} TZinfo;
\n    ");

printf $ch (
"const TZinfo TZall[] PROGMEM = {\n    ");


my $content = <$fh>;

my $t=0;
my $n=0;
my $c=0;
my $k='';
my %decode;
my $last_zone;
my $last_c;
my $delim;

foreach my $line (split /[\r\n]+/, $content) {
    $line =~ s/\"//g; # strip out quotes
    my @words = split /,/,$line;
    my $tz = $words[0];
    if ( "$tz" !~ /$re/ )  {  next;  }
    $tz =~ s/\//,/; # swap out first slash
    my @tz = split /,/,$tz;
    if (  $k ne $tz[0] ) {
        if( $n == 0 ) { printf $js ("   var "); }
      $k = $tz[0];
      #printf $html ("      <optgroup label=\"%s\" data-gropu-exclusive>\n",$k);
      printf $js ("   grp = \$(\"<optgroup />\", {tzid: \"%s\", class: \"tzZone\", label: \"%s\", vis: \"0\"}).appendTo(cit);\n", $k,$k);
    }

    my $tzname = $words[1];
    my $start = $words[2];
    my $end = $words[3];
    my $def_need = 0;

    if ( defined($decode{$tzname . $start . $end} ) ) {
        $c = $decode{$tzname . $start . $end};
        $def_need = 0;
    } else {
        $def_need = 1;
        $c = $n;
        #
        # comment out the next line for all entries to be unique
        #
        if ( !$all ) { 
            $decode{$tzname . $start . $end} = $n;
        }
    }
    #printf $html ("         <option value=\"%d\">%s</option>\n",$c,$tz[1]);
    printf $js ("   \$(\"<option />\", {value: \"%d\", text: \"%s\", tzzone: \"%s\", class: \"tz\"}).appendTo(grp);\n", $c,$tz[1],$k);

    #printf( "%s/%s %s %s %d %s %s %d %s %s\n",$k,$tz[1], $tzn, $tzone, $tzone_min, $tzs, $tzsn, $tzsn_min, $start, $end);
    if ( $def_need ) {
        my $tzn = '';
        my $tzone = 0;
        my $tzone_h = 0;
        my $tzone_m = 0;
        my $tzone_min = 0;
        my $tzs = '';
        my $tzsn = '';
        my $tzsn_min = '';
        if ( $tzname =~ /([A-Z]+)([-+]*[0-9:]+)([A-Z]+)?(.*)/ ) {
            $tzn = $1; $tzone = $2; $tzs = $3; $tzsn = $4;
        } elsif ( $tzname =~ /(<[^>]+>)([-+]*[0-9:]+)(<[^>]+>)?(.*)/ ) {
            $tzn = $1; $tzone = $2; $tzs = $3; $tzsn = $4;
        }
        $tzone =~ /([-+])([0-9]+)(:([0-9]+))?/;
        $tzone_h = $2;
        $tzone_m = $4;
        $tzone_min = ($tzone_h * 60) + $tzone_m;
        if ( $1 ne "-" ) { $tzone_min = - $tzone_min; }
        if ( $tzsn == "" ) {
            $tzsn = $tzone_min + 60;
        } else {
            $tzsn =~ /([-+])([0-9]+)(:([0-9]+))?/;
            $tzsn_min = ($2 * 60) + $4;
            if ( $1 ne "-" ) { $tzsn_min = - $tzsn_min; }
            if ( $2 > 24 ) { 
                $tzsn = $tzone_min + 60;
            }
        }
        if( $n++ != 0 ) { printf $ch (", %s\n    ", $last_zone);};
        $last_zone = "";
        if ( $#words == 1 ) { # no dst
            printf $ch ("{\"%s\",%d, NULL, 0, 0, 0, 0, 0, 0, 0}", $tzn, $tzone_min);
        } else {
	    my $m;
	    my $w;
	    my $d;
            printf $ch ("{\"%s\",%d, \"%s\", %d, ", $tzn, $tzone_min, $tzs, $tzsn);
            if ( $start =~ /M([0-9]+)\.([0-5])\.([0-6])\/?(.*)/ ) {
		$m = $1;
		$w = $2;
		$d = $3;
		if ($4 =~ /([0-9]+)(:([0-9]+))?/ ) {
		    $t = (($1 % 24) * 60) + $3;
		} else { $t = 120; }
	    } elsif ( $start =~ /(J)?([0-9]+)\/?(.*)/ ) {
		$m = $2;
		if ( $1 eq "J" ) {
		  $d = 7;
		} else {
		  $d = 8;
		}
		$w = 0;
		if ( $m > 200 ) {
		    $w = $m - 200;
		    $m = 200;
		}
		if ($3 =~ /([0-9]+)(:([0-9]+))?/ ) {
		    $t = (($1 % 24) * 60) + $3;
		} else { $t = 120; }
	    }
	    printf $ch ("%d, %d, %d, %d, ", $m, $w, $d, $t);
            if ( $end =~ /M([0-9]+)\.([0-5])\.([0-6])\/?(.*)/ ) {
		$m = $1;
		$w = $2;
		$d = $3;
		if ($4 =~ /(-?[0-9]+)(:([0-9]+))?/ ) {
		    $t = (($1) * 60) + $3;
		} else { $t = 120; }
	    } elsif ( $end =~ /(J)?([0-9]+)\/?(.*)/ ) {
		$m = $2;
		if ( $1 eq "J" ) {
		  $d = 7;
		} else {
		  $d = 8;
		}
		$w = 0;
		if ( $m > 200 ) {
		    $w = $m - 200;
		    $m = 200;
		}
		if ($3 =~ /(-?[0-9]+)(:([0-9]+))?/ ) {
		    $t = (($1) * 60) + $3;
		} else { $t = 120; }
	    }
	    printf $ch ("%d, %d, %d, %d}", $m, $w, $d, $t);
        }
    } else {
        $last_zone = $last_zone . "\n    ";
    }
    $last_zone = $last_zone . "// " . $k . "/" . $tz[1] . " = " . $c;
}
#printf $html ("      </optgroup>\n   </select>\n");
printf $ch ("%s\n    ", $last_zone);
printf $ch ("};\n");
printf $ch ("#define MAX_TIME_ZONE %d\n",$n-1);
printf $js ("

     \$( \"optgroup\" ).click(function() {
         var value = \$(this).attr(\"tzid\");
         var vis = \$(this).attr(\"vis\");
         \$(\".tz\").hide();
         if ( vis === \"0\" ) {
           \$(\"[tzzone=\" + value + \"]\").show();
           \$(this).attr(\"vis\",1);
         } else {
           \$(this).attr(\"vis\",0);
         }
         \$(\":selected\").show();
     });\n}\n
\$(function() {
    loadAllTimeZones();
    \$(\".tz\").hide();
});
")
