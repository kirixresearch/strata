
all: imgres.zip

imgres.zip: $(basedir)\imgres.zip

$(basedir)/imgres.zip:
	..\zip -0 $(basedir)\imgres.zip *.xpm *.png *.gif
	IF EXIST c rmdir c /s /q
	mkdir c
	FOR %%X IN (*.png) DO CALL .\processimg %%X

clean:
	IF EXIST $(basedir)\imgres.zip erase $(basedir)\imgres.zip
