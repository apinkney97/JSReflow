directory ../lib

define redo
symbol-file imgrotate
exec-file imgrotate
end

set args -verbose -flip -output-file=../fonts/ctst.img ../fonts/old-ctst
#set args -verbose -flip -output-file=../images/ggmr.img ../images/180/ggmr
#set args -verbose -flip -output-file=../images/ggmri.img ../images/180/ggmri
#set args -verbose -rotate-clockwise -output-file=msg.img msgrot
#set args -verbose -flip -output-file=msgrot.img msgrotups
#set args -verbose -flip -output-file foo.bar ../ourfonts/img/ctst
#set args -verbose -flip -output-file foo ../ourfonts/img/ctst
set args -verbose -flip  ../ourfonts/img/ctst
