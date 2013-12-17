usr-sh	:yacc.tab.o	execute.o
	cc -o user-sh yacc.tab.o execute.o
yacc.tab.o:	yacc.tab.c	global.h
	cc -c -g yacc.tab.c
execute.o:	execute.c global.h jobs.h
	cc -c -g execute.c
yacc.tab.c: yacc.y
	bison -y -d yacc.y -o yacc.tab.c
clean:
	rm user-sh yacc.tab.o execute.o
