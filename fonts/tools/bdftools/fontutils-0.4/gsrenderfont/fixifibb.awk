# We assume the variable `bbsfile' has been set from the command line,
# and is the name of a bbs file output by bbcount.  We combine it and an
# xifi file (output by writefont.PS) on the input, and write
# an ifi file on stdout.

	{ getline bbcount < bbsfile;
          print $1, $2, bbcount, $4, $5
        }
