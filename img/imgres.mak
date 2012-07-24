
all: imgres.zip

imgres.zip: $(basedir)/imgres.zip

$(basedir)/imgres.zip:
	..\zip -0 $(basedir)/imgres.zip *.xpm *.png *.gif

clean:
	IF EXIST $(basedir)\imgres.zip erase $(basedir)\imgres.zip
