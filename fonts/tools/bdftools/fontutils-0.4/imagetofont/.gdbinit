directory ../gf
directory ../pbm
directory ../pk
directory ../lib

define redo
symbol-file imagetofont
exec-file imagetofont
end

#set args -verbose -input-format=pbm -dpi=300 -design-size=10 hvr
#set args  -input-format=pbm -dpi=300 -design-size=36 a
#set args -verbose -encoding-file=encoding.bdk -baselines=285,301,399 \
#          -clean-threshold=.4 -print-guidelines bdk
#set args -verbose -encoding-file=encoding.bdb -baselines 265,280,296 bdb
# Some junk is between the first two real lines, and a black bar is after
# the last.  Hence the two extra baselines.
#set args -verbose -encoding-file=encoding.bdki -baselines=286,12,293,322,1 \
#  -clean-threshold=25 -range=I-I -nchars=1 -print-clean-info bdki
#set args -verbose -encoding-file=encoding.tmi -print-guidelines \
#  -print-clean-info -design-size=72 -clean-threshold=30 -nchars=6 tmi
#set args -verbose -baselines=327,338,342 -clean-threshold=23 \
#  -design-size=30 ../images/ggmr
#set args -verbose -baselines=1,335,1,340,333 -clean-threshold=31 \
#  -design-size=30 ../images/ggmri
#set args -verbose -strips ../images/ggmi
#set args -verbose -baselines=154,156,155 \
#  -design-size=14 ../images/ggmb
#set args -verbose -baselines=152,158,160,161,162 -clean-threshold=36 \
#  -design-size=14 ../images/ggmbi
#set args -input-format=img -verbose -design-size=64 msg.img
#set args ../gsrenderfont/phvr -info-file=../gsrenderfont/phvr.ifi \
#  -dpi=300 -input=pbm -verbose -output=phvr.300gf #\
#  -trace-scanlines 
#set args ../gsrenderfont/phvro -info-file=../gsrenderfont/phvro.ifi \
#  -dpi=360 -input=pbm -verbose -output=phvro.360gf #\
#  -trace-scanlines 
#set args -print-guidelines -design-size=30 -dpi 1200 -input-format=img \
#  -verbose -info-file ../data/09.ifi -baselines 57 ../img/09.img
#set args -print-guidelines -design-size=30 -dpi 1200 -input-format=img \
#  -verbose -info-file ../data/09.ifi -output-file ../fonts/0930a \
#  -baselines 57 ../img/09.img
#set args -print-guidelines -design-size=30 -dpi 1200 -input-format=img \
#  -verbose -info-file ctst -output-file ../fonts/ggmc ../img/ctst.img
#set args -print-guidelines -design-size=30 -dpi 1200 -input-format=img \
#  -verbose -info-file 09 ../fonts/09.img

#set args -design-size=30 -dpi 1200 -input-format=img -verbose \
#    -strips -output-file foo.bar ../ourfonts/img/ctst.img
#set args -design-size=30 -dpi 1200 -input-format=img -verbose \
#     -info-file ../ourfonts/ifi/ctst -output-file foo.bar \
#     ../ourfonts/img/ctst.img

#set args -design-size=30 -dpi 1200 -input-format=img -verbose  \
#     -strips -output-file foo ../ourfonts/img/ctst.img
#set args -design-size=30 -dpi 1200 -input-format=img -verbose  \
#     -info-file ../ourfonts/ifi/ctst -output-file foo ../ourfonts/img/ctst.img

#set args -design-size=30 -dpi 1200 -input-format=img -verbose  \
#     -strips  ../ourfonts/img/ctst.img
set args -design-size=30 -dpi 1200 -input-format=img -verbose \
    -info-file ../ourfonts/ifi/ctst ../ourfonts/img/ctst.img

