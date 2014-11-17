directory ../lib
directory ../gf
directory ../pk
directory ../tfm

define redo
symbol-file fontconvert
exec-file fontconvert
end

#set args -verbose -remap=::a,000:-1,,a:b,b:: cmr10
#set args pbkl -dpi=85 -verbose -gf \
#-omit 0,9,0240,0241,0255,0261,0262,0263,0266,0267,0270,0271,0272,0275,\
#0302,0303,\
#0305,0306,0312,0313,0314,0315,0322,0323,0324,0350,0351,0352,0353,0361 \
#-remap 0245:0267,0246:0266,0247:0373,\
#0250:0322,0251:0323,0252:0324,0253:0302,0254:0310,0256:0341,0257:0351,\
#0264:0245,\
#0273:0343,0274:0353,0277:0371,\
#0301:0241,0304:0240,0307:0253,\
#0310:0273,0316:0352,0317:0372,\
#0341:0264,0343:0271,\
#0370:0305,0371:0306,0372:0307,0373:0312
#set args -verbose -text cmr10 > cmr10.typ
#set args -verbose -gf -filter -dpi=1200 ../ourfonts/ggmri30a 
#set args -verbose -range=0220-0224 -text ../imgtogf/ggmbi14.1200 > /tmp/out
#set args -verbose -gf -column-split=v@114,285,395,499 ../imgtogf/ggmb14x.1200
#set args -verbose -gf -filter -ou ../ourfonts/ggmb14b ../ourfonts/ggmb14a.1200
#set args -verbose -gf -column-split=0220@188 -column-split=0224@174 \
#  ../imgtogf/ggmbi14.1200
#set args -verbose -gf -filter -ou ../ourfonts/ggmri30c ../ourfonts/ggmri30a.1200
#set args -gf -concat logo10 euex10.118
#set args -gf -tfm -baseline-adjust=a:-8,c:-8,e:-6,f:-1,h:-2,i:-2,k:-2,l:-1,\
#m:-1,n:-1,o:-2,r:-1,x:-1,z:-1 \
#  -verbose -ou=../ourfonts/ggmr30i ../ourfonts/ggmr30h.1200
#set args -verbose \
#  -fontdimens=1:3.4,space:4.9 -fontdimens=xheight:.6,17:28.1 -tfm \
#  dingbat
#set args -verbose -tfm \
#  -fontdimens=space:10,stretch:3,shrink:1,xheight:12,quad:30,\
#extraspace:2 \
#  ../ourfonts/ggmr30i.1200
#set args -verbose -gf ../ourfonts/ggmr30i.1200gf \
#  -output-file=../ourfonts/ggmr30j \
#  -remap=0243:0277,0256:034,0257:035,0260:033,0326:036,0327:037,\
#0341:0306,0352:0327,0361:0346,0372:0367
#set args -verbose -epsf ./msg64.300gf
#set args -remap 060:0200,061:0201,062:0202,063:0203,064:0204,065:0205,\
#  066:0206,067:0207,070:0210,071:0211,033:013,034:014,035:015,036:016,\
#  037:017,0277:0243,0327:036,0367:033 \
#  -dpi 1200 -gf -output-file foo -verbose ../charspace/ggmr30n 
#set args -dpi 1200 -tfm -output-file ../ourfonts/0930 ../imagetofont/0930.1200gf
#set args -dpi 1200 -tfm -output-file ../ourfonts/ctst30 ../imagetofont/ctst30.1200gf
#set args -dpi 1200 -tfm -output-file ../ourfonts/ctst30 ../imagetofont/ctst30.1200gf
#set args -dpi 1200 -gf -output-file ../ourfonts/ggmr30p \
#  -concat ../ourfonts/0930.1200gf ../ourfonts/ggmr30o.1200gf
#set args -dpi 1200 -gf -tfm -output-file ../ourfonts/ggmr30p -verbose\
#  -concat ../ourfonts/0930.1200gf ../ourfonts/ggmr30o.1200gf
set args -random=1 -random-thresh=.1 -gf -tfm -output-file=cmr10-11 -verbose cmr10
#set args -random=4 -gf -tfm -output-file=cmr10-4 -verbose cmr10
#set args -dpi 1200 -baseline-adjust 0372:10,0373:10 -tfm -gf \
#  -output-file ../ourfonts/ggmc30b ../ourfonts/ggmc30a.1200gf

# test output files.
#set args -verbose -dpi 1200 -tfm -output-file foo.tfm ../ourfonts/0930.1200gf
#set args -verbose -dpi 1200 -epsf -output-file foo ../ourfonts/0930.1200gf
#set args -verbose -dpi 1200 -epsf -output-file foo.bar ../ourfonts/0930.1200gf
#set args -verbose -dpi 1200 -gf -tfm -output-file foo.bar ../ourfonts/0930.1200gf
#set args -verbose -dpi 1200 -gf ../ourfonts/0930.1200gf
#set args -verbose -dpi 1200 -tfm ../ourfonts/0930.1200gf

#set args -verbose -dpi 1200 -tfm -output-file ../ourfonts/ggmr30s \
#	../ourfonts/ggmr30s

# test -tfm-header option
#set args -verbose -dpi 1200 -tfm -output-file ../ourfonts/foo30 \
#  	-tfm-header checksum:5,designsize:24,codingscheme:fooscheme \
#	-fontdimens slant:3,space:2 ../ourfonts/foo30.1200gf
#set args -verbose -dpi 1200 -tfm -output-file ../ourfonts/foo30 \
#  	-tfm-header checksum:5,designsize:24,codingscheme:fooscheme \
#        ../ourfonts/foo30.1200gf

# test -design-size, checksum zeroing
#set args -verbose -dpi 1200 -tfm -gf -output-file ../ourfonts/foo30 \
#  	-tfm-header designsize:20 -design-size 30 ../ourfonts/foo30.1200gf


# scale ggm*30 to ggm*26 with TFM header only
#set args -verbose -dpi 1200 -tfm -output-file ../ourfonts/ggmr26a \
#  	-tfm-header designsize:26 ../ourfonts/ggmr30u
#set args -verbose -dpi 1200 -tfm -output-file ../ourfonts/ggmc26a \
#	-tfm-header designsize:26 ../ourfonts/ggmc30b

# make a small tfm files with ligatures fi, fl, among other things.
#set args -verbose -range 013-l -tfm -gf -output-file ffl10 cmr10
# omit l, so no more fl or ffl.
#set args -verbose -range 013-i -tfm -gf -output-file ffi10 cmr10
# omit ff, so no ffi, ffl.
#set args -verbose -range 014-l -tfm -gf -output-file fl10 cmr10
# should delete all kerns but 67 for 65.
#set args -verbose -range 65-67 -tfm -gf -output-file ac10 cmr10
# remap 65's kern 67 character to 68.
#set args -verbose -range 65-67 -remap 67:68 -tfm -gf -output-file ad10 cmr10
# remap 11's lig character 108 to 109 and its destination 15 to 16.
#set args -verbose -range 11-108 -remap 108:109,15:16 -tfm -gf \
#  -output-file rffl10 cmr10

# test -design-size option.
#set args -verbose -tfm -gf -output-file ds -range a-a \
#  -tfm-header designsize:0 cmr10
#set args -verbose -tfm -gf -output-file ds -range a-a -design-size 0 \
#  -tfm-header checksum:1 cmr10
#set args -verbose -tfm -gf -output-file ds -range a-a -design-size 0 crm10
#set args -verbose -tfm -gf -output-file ds -range a-a -design-size 12 cmr10

# scale ggm*30 to ggm*26 with global -design-size option.
#set args -verbose -dpi 1200 -tfm -gf -output-file ../ourfonts/ggmr26a \
#  	-design-size 26 ../ourfonts/ggmr30u
#set args -verbose -dpi 1200 -tfm -gf -output-file ../ourfonts/ggmc26a \
#	-design-size 26 ../ourfonts/ggmc30b

# add space to ggmr26b->ggmr26c
#set args -verbose -dpi 1200 -tfm -gf -output-file ../ourfonts/ggmr26c \
#  	-fontdimens space:11.5  ../ourfonts/ggmr26b

# reduce space of ggmr26d->ggmr26eb
set args -verbose -dpi 1200 -tfm -gf -output-file ../ourfonts/ggmr26c \
  	-fontdimens space:11.5  ../ourfonts/ggmr26b

