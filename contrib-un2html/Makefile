all: un2html un2tbl

CFLAGS= -g -I/opt/exp/lib/unity/lib  
CC=gcc

UN2HTML_OBJS=Strcat.o Strsrch.o Set_option.o Pwd.o Catch.o Strcpy.o Fopen.o \
  Quit.o Digtostr.o Clear_opts.o Curr_date.o   un2html.o

un2html: $(UN2HTML_OBJS)
	$(CC) $(UN2HTML_OBJS) /opt/exp/lib/unity/lib/dblib.a $(LIBS) -o $@

UN2TBL_OBJS=Strcat.o Strsrch.o Pwd.o Catch.o Strcpy.o Fopen.o \
  Quit.o Digtostr.o Clear_opts.o Curr_date.o   un2tbl.o 

un2tbl: $(UN2TBL_OBJS)
	$(CC) $(UN2TBL_OBJS) /opt/exp/lib/unity/lib/dblib.a $(LIBS) -o $@

install: un2html un2tbl
	[ -d bin ] || mkdir bin
	[ -d lib ] || mkdir lib
	[ -d hdr ] || mkdir hdr
	cp un2html$(EXEEXT) bin
	cp un2tbl$(EXEEXT)  bin
	strip bin/*

clean:
	rm -f *.o core un2tbl un2html 

clobber: clean
	rm -rf bin
