.SUFFIXES:	.ps .enc .afm .sel .h

.ps.h:
		sed -e 's/.*/"&",/' $*.ps >$*.h

.enc.h:
		sed -e 's/.*/{"&"},/' $*.enc >$*.h

.sel.afm:
		hersel $*.sel hersh.oc

AFM =	HerGotEng.afm HerGotEngB.afm HerGotEngO.afm HerGotGer.afm \
	HerGotGerB.afm HerGotGerO.afm HerGotIta.afm HerGotItaB.afm \
	HerGotItaO.afm HerPla.afm HerPlaB.afm HerPlaO.afm HerScrCom.afm \
	HerScrComB.afm HerScrComO.afm HerScrSim.afm HerScrSimB.afm \
	HerScrSimO.afm HerTri.afm HerTriB.afm HerTriBI.afm HerTriI.afm

fonts:		$(AFM)
		touch fonts
$(AFM):		hersel hersh.oc
hersh.oc:	hersh.oc1 hersh.oc2 hersh.oc3 hersh.oc4
		cat hersh.oc[1234] >hersh.oc

hersel:		hersel.o
		cc -o hersel hersel.o
hersel.o:	standard.h makefont.h

herps:		herps.o
		cc -o herps herps.o
herps.o:	prolog.h pagestart.h pagefinish.h trailer.h
