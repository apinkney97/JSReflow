directory ../gf
directory ../widgets
directory ../lib
directory ../pk
directory ../tfm

#directory /usr/local/src/X/mit/lib/Xaw
#directory /usr/local/src/X/mit/lib/Xt
#directory /usr/local/src/X/mit/lib/X

define redo
symbol-file xbfe
exec-file xbfe
end

#set args  -initial-char=M cmr10.1200
#set args -dpi=1200 torch

set args -initial-char=A foo.300

# For horizontal scrollbar only:
# set args -geometry 200x1000-0-0 cmr10.300

# For vertical scrollbar only:
#set args -geometry 700x200-0-0 cmr10.300 -output-file=temp

#set args -geometry 1200x1200-0-0 -initial-char=l ../fonts/ggmr30m.1200gf \
#  -output=temp.1200gf
#set args -geometry 1200x1200-0-0 -initial-char=l temp.1200 \
#  -output=temp2.1200gf
#set args -initial-char=B msg64.300
#set args -initial-char=A msgru64.300

#set args -initial-char=f x.204 -output=temp

#set args -dpi 1200 -initial-char 0 -output-file ../fonts/ggmr30s.1200gf \
#  ../font/ggmr30r

