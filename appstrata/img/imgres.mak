
all: imgres.zip

imgres.zip: $(outdir)\imgres.zip

$(outdir)/imgres.zip:
	$(basedir)\supp\util\zip -0 $(outdir)\imgres.zip *.svg *.png

clean:
	IF EXIST $(outdir)\imgres.zip erase /q /f $(outdir)\imgres.zip
