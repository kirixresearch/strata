
all: imgres.zip

imgres.zip: $(basedir)/imgres.zip

$(basedir)/imgres.zip:
	..\zip -0 $(basedir)/imgres.zip *.xpm *.png *.gif
	IF NOT EXIST .\c mkdir c
	FOR /f %%X IN ('dir /b *.png') DO ..\bin2c -o c\%%X.c %%X

clean:
	IF EXIST $(basedir)\imgres.zip erase $(basedir)\imgres.zip
