static char *SCCSID="@(#)un2tbl.c	2.1.1.7";
/* 
 * Unity to tbl converter
 *
 * Written by R. de Heij
 * LastEditDate="Fri Jul  5 01:06:14 1996"
 */

#include <math.h>
#include <stdio.h>
#include <db.h>

extern void Clear_opts();
extern void Catch();
extern void Prints();
extern char *Curr_date();
extern char *strcpy();
extern char *Strsrch();
extern FILE *Fopen();
extern double atof();

main(argc, argv)
char *argv[];
{
    FILE           *utfp1;
    struct fmt      xx[MAXATT];
    float     	    with_spec[MAXATT];
    float inch;
    int             nattr1, endf1, recordnr, i;
    char            Dtable1[MAXPATH], *prog, **p;
    int             inptr, whereptr;
    int             prtit[MAXATT];     /* Print yes or no */
    int             x;		       /* help var */
    int 	    endofanames;
    char *sptr;
    int tblwidth;
    int overrule_tblwidth;

    recordnr = endf1 = 0;
    prog = argv[0];

    /* Catch interrupts */
    Catch();

    /* Usage */
    if ( argc < 2 ) {
	fprintf(stderr, "usage: un2tbl [<anames> in] <UNITY_database> [<UNITY_where_clause>]\n");
	exit(1);
    }

    /* Reset arguments */
    Clear_opts("", &argc, &argv);

    /* Parse input */
    inptr = 0;
    endofanames = 0;
    whereptr = argc;
    for (i = 0; i < argc; i++) {
	if (strcmp(argv[i], "in") == 0){
	    inptr = i + 1;
	    endofanames = i;
	}
	if (strcmp(argv[i], "where") == 0)
	    whereptr = i;
    }

    /* Open table */
    utfp1 = Fopen(argv[inptr], "r");

    /* Load D  file */
    if ((nattr1 = mktbl(prog, argv[inptr], Dtable1, xx)) == ERR)
	Quit("Dfile format error", 0);
    if (xx[nattr1 - 1].flag == WN)
	endf1 = 1;

    /* Build table with attributes to use */
    for (i = 0; i < nattr1; i++) {
	prtit[i] = 0;
    }
    for (i = 0; i < endofanames; i++) {

	inch = 0.0;

    	if((sptr=Strsrch(argv[i],"!")) != NULL){
		inch=(float)atof(sptr+1);
		*sptr = 0;
	}

	overrule_tblwidth=0;
    	if((sptr=Strsrch(argv[i],":")) != NULL){
		overrule_tblwidth=1;
		tblwidth=atoi(sptr+1);
		*sptr = 0;
	}

	if ((x = setnum(xx, argv[i], nattr1)) == ERR) {
	    error(E_ILLATTR, prog, argv[i], Dtable1);
	    Quit(0);
	}
	prtit[x] = 1;
	if(overrule_tblwidth)
		xx[x].prnt=tblwidth;

	with_spec[x] = inch;
    }

    /* Get Where */
    p = &argv[whereptr];
    getcond(p, xx, nattr1, prog, Dtable1);
    recordnr = 0;

    /* Print Table header */
    fprintf(stdout, ".TS H\n");
    fprintf(stdout, "expand box tab(@);\n");
    for (x = 0, i = 0; i < nattr1; i++) {
	if (endofanames == 0 || prtit[i] == 1) {
	    if (x++ > 0)
		fprintf(stdout, " ");
	    if(with_spec[i] > 0.0)
		    fprintf(stdout, "%cw(%.2fi)", xx[i].justify,with_spec[i]);
	    else
		    fprintf(stdout, "%c", xx[i].justify);
	}
    }
    fprintf(stdout, ".\n");
    for (x = 0, i = 0; i < nattr1; i++) {
	if (endofanames == 0 || prtit[i] == 1) {
	    if (x++ > 0)
		fprintf(stdout, "@");
	    fprintf(stdout, "%s", xx[i].aname);
	}
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "=\n");
    fprintf(stdout, ".TH\n");

    /* Go through it */
    for (;;) {
	newrec();
	for (i = 0; i < nattr1 && getrec(utfp1, &xx[i]) != ERR; i++);
	if (i < nattr1)
	    break;
	if (endf1)
	    while (!feof(utfp1) && getc(utfp1) != '\n');
	recordnr++;
	if (selct(xx, recordnr)) {
	    for (x = 0, i = 0; i < nattr1; i++) {
		if (endofanames == 0 || prtit[i] == 1) {
		    if (x++ > 0)
			fprintf(stdout, "@");
		    if (strlen(xx[i].val) > xx[i].prnt) {
			fprintf(stdout, "T{\n%s\nT}", xx[i].val);
		    } else {
			fprintf(stdout, "%s", xx[i].val);
		    }
		}
	    }
	    fprintf(stdout, "\n");
	}
    }
    /* Print Table header */
    fprintf(stdout, ".TE\n");
    return (0);
}
