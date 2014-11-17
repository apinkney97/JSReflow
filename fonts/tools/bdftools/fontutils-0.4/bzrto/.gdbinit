directory ../bzr
directory ../tfm
directory ../lib

define redo
symbol-file bzrto
exec-file bzrto
end

#set args -ascii -pstype3 bsq > bsq.typ
#set args -pstype3 ../fit-outlines/ggmr-0
#set args -metafont ../fonts/ggmr30l
#set args -verbose -metafont -pstype3 cmr12
#set args -verbose -text -pstype3 -pstype1 -metafont cmr10 > cmr10.txt
#set args -verbose -text -pstype3 -pstype1 -metafont test10 > test.txt
#set args -verbose -metafont -pstype1 -output-file foo.bar cmr10
#set args -verbose -metafont -pstype3 -output-file foo.bar cmr10
#set args -verbose -metafont -pstype1 cmr10
#set args -verbose -text cmr10 > cmr10.txt
#set args -verbose -metafont -output-file foo.mf cmr10
set args -verbose -metafont -pstype1 -output-file ../ourfonts/ggmr26D \
  ../ourfonts/ggmr26D
