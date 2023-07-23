#!/usr/bin/perl
use strict;
use warnings;

if($#ARGV != 0) {
   print "Usage: cvt.pl filename\n";
   exit;
}

my $dir = $ARGV[0];
my $dstfile;
my $srcfile;
my $header;

opendir DIR, $dir;
my @dir = readdir(DIR);
close DIR;
foreach(@dir){
    if(m/(.+)\.h$/) {
        $dstfile = $_;
        $srcfile = $dir . "\\" . $_;
        $header = "__DUI_" . uc($1) . "_H__";
        
        open(DSTFILE, '>', $dstfile) or die "Could not open file '$dstfile' $!";
        print "$srcfile ($header) is processed!\n";
        print DSTFILE "#ifndef $header\n";
        print DSTFILE "#define $header\n\n\n";
        print DSTFILE "#endif /* $header */\n";
        # print DSTFILE "//include \"$1.hpp\"\n";
        print DSTFILE "\n\n\n#if 0\n/*====================================================================*/\n";
        open(SRCFILE, '<', $srcfile) or die "Could not open file '$dstfile' $!";
        while (<SRCFILE>) {
            print DSTFILE $_;
        }
        print DSTFILE "/*====================================================================*/\n#endif\n\n";
        
        close(SRCFILE);
        close(DSTFILE);
    } elsif(m/(.+)\.cpp$/) {
        $dstfile = $1 . "\.cpp";
        $srcfile = $dir . "\\" . $_;
        $header = "__DUI_" . uc($1) . "_H__";
        
        open(DSTFILE, '>', $dstfile) or die "Could not open file '$dstfile' $!";
        print "=============== $srcfile ($header) is processed!\n";
        print DSTFILE "\n\n\n#if 0\n/*====================================================================*/\n";
        open(SRCFILE, '<', $srcfile) or die "Could not open file '$dstfile' $!";
        while (<SRCFILE>) {
            print DSTFILE $_;
        }
        print DSTFILE "/*====================================================================*/\n#endif\n\n";
        
        close(SRCFILE);
        close(DSTFILE);
    }
}

exit;
