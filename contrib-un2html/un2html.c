static char *SCCSID="@(#)un2tbl.c	2.1.1.6";
/* 
 * Unity to tbl converter
 *
 * Written by R. de Heij
 * LastEditDate="Sat Oct 11 11:45:54 1997"
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

/* Globals */
struct fmt  dfile_info[MAXATT];
int         nr_attrs_in_dfile;
char *i_opt="";
char *M_opt="";
char *C_opt="";
char *A_opt="";


href_prt(href, attr)
char *href;
char *attr;
{
    char           *optr;
    char            part[MAXREC];
    char            crd[MAXREC];
    char            ar[MAXREC];

    /* IMR */
    if (strcmp(href, "%I") == 0) {
	while (*attr != 0) {
	    optr = part;
	    while (*attr && *attr != ',')
		*optr++ = *attr++;
	    *optr = 0;
	    fprintf(stdout, "<a href=\"");
	    templ_prt(i_opt, part);
	    fprintf(stdout, "\">%s</a> ", part);
	    while (*attr == ',')
		attr++;		       /* SKIP seps */
	    if (*attr)
		fprintf(stdout, ", "); /* another will follow */
	}
    } else {
	/* MR */
	if (strcmp(href, "%M") == 0) {
	    while (*attr != 0) {
		optr = part;
		while (*attr && *attr != ',')
		    *optr++ = *attr++;
		*optr = 0;
		fprintf(stdout, "<a href=\"");
		templ_prt(M_opt, part);
		fprintf(stdout, "\">%s</a>", part);
		while (*attr == ',')
		    attr++;	       /* SKIP seps */
		if (*attr)
		    fprintf(stdout, ", ");	/* another will follow */
	    }
	} else {
	  if (strcmp(href, "%A") == 0)
	  {
	    /* AR CARES ticket */
                while (*attr != 0) {
                    optr = part;
                    while (*attr && *attr != ',')
                        *optr++ = *attr++;
                    *optr = 0;
                    fprintf(stdout, "<a href=\"");
                    templ_prt(A_opt, part);
                    fprintf(stdout, "\">%s</a>", part);
                    while (*attr == ',')
                        attr++;        /* SKIP seps */
                    if (*attr)
                        fprintf(stdout, ", ");  /* another will follow */

	  	}
	   } else {
	    /* CAROD */
	    if (strcmp(href, "%C") == 0) {
		while (*attr != 0) {
		    optr = part;
		    while (*attr && *attr != ',')
			*optr++ = *attr++;
		    *optr = 0;
		    fprintf(stdout, "<a href=\"");
		    templ_prt(C_opt, part);
		    fprintf(stdout, "\">%s</a>", part);
		    while (*attr == ',')
			attr++;	       /* SKIP seps */
		    if (*attr)
			fprintf(stdout, ", ");	/* another will follow */
		}
	    } else {
		/* AUTO DETECT IMR CAROD or AR  (IC and ICA) */
		if ( (strcmp(href, "%IC") == 0) || 
		     (strcmp(href, "%ICA") == 0)    )
		   {
		    while (*attr != 0) {
			optr = part;
			while (*attr && *attr != ',')
			    *optr++ = *attr++;
			*optr = 0;
			if (strlen(part) == 6 && part[0] != '9' &&  part[0] != '0' ) {
			    fprintf(stdout, "<a href=\"");
			    templ_prt(i_opt, part);
			    fprintf(stdout, "\">%s</a>", part);
			} else {
			   if ( part[0] == '0' )
				{   /* CARES AR ticket */
				    strcpy(ar, part);
				    if (strlen(part) == 5)
					Strcpy(ar, "1-00", part, 0);
				    if (strlen(part) == 6 && part[0] == '0')
					Strcpy(ar, "1-00", part + 1, 0);
				    fprintf(stdout, "<a href=\"");
				    templ_prt(A_opt, ar);
				    fprintf(stdout, "\">%s</a>", ar);
				} else 	{
				     /* CAROD ticket */
				    strcpy(crd, part);
				    if (strlen(part) == 5)
					Strcpy(crd, "IH", part, 0);
				    if (strlen(part) == 6 && part[0] == '9')
					Strcpy(crd, "IH", part + 1, 0);
				    fprintf(stdout, "<a href=\"");
				    templ_prt(C_opt, crd);
				    fprintf(stdout, "\">%s</a>", crd);
				}
			}
			while (*attr == ',')
			    attr++;    /* SKIP seps */
			if (*attr)
			    fprintf(stdout, ", ");	/* another will follow */
		    }
		} else {
		    fprintf(stdout, "<a href=\"");
		    templ_prt(href, attr);
		    if (strlen(attr) > 0) {
			fprintf(stdout, "\">%s</a>", attr);
		    } else {
			fprintf(stdout, "\">-</a>");
		    }
		}
	      }
	    }
	}
    }
}

templ_prt(templ,attr)
char *templ;
char *attr;
{
    char           *sptr;
    char           *varptr;
    char           *optr;
    char            var[256];
    char            cstring[MAXREC];
    char            rest[MAXREC];
    int             i;

    strcpy(cstring, templ);
    sptr = Strsrch(cstring, "${");
    while (sptr != NULL) {
	varptr = sptr + 2;
	optr = var;
	while (*varptr && *varptr != '}') {
	    *optr++ = *varptr++;
	}
	*optr = 0;
	if (strcmp(var, "%") == 0) {
	    strcpy(rest, varptr + 1);
	    Strcpy(sptr, attr, rest, 0);
	} else {
	    if ((i = setnum(dfile_info, var, nr_attrs_in_dfile)) == ERR) {
		sptr = varptr;
	    } else {
		strcpy(rest, varptr + 1);
		Strcpy(sptr, dfile_info[i].val, rest, 0);
	    }
	}
	sptr = Strsrch(sptr, "${");
    }
    fprintf(stdout, "%s", cstring);
}

main(argc, argv)
int argc;
char *argv[];
{
    extern int      optind;

    int             x, i;	       /* help var */
    char           *sptr;
    int             endf1, recordnr;
    char           *prog;
    char           *t_opt = "";	       /* t option */
    char           *I_opt = NULL;      /* I option */
    FILE           *utfp1;
    struct out {		       /* Attribute formatting */
	char           *href;
	char           *th;
	char           *td;
	char           *templ;
	int             attr;
    }               todo[MAXATT];
    int             nr_todo = 0;       /* Attributes to print */
    char            Dtable1[MAXPATH], **p;
    int             in_ptr, where_ptr;
    int             endofanames;

    recordnr = endf1 = 0;
    prog = argv[0];

    /* Catch interrupts */
    Catch();

#define OPTS    "t:I:C:A:T:M:i:"

    /* Usage */
    if ( argc < 2 ) {
	fprintf(stderr,"usage: un2html [-I<dfile>] [-t<Table_options>] [-M<MR_cgi>]\n   [-C<Carod_cgi>] [-A<Cares_cgi>] [-i<Imr_cgi>] [<anames> in]\n   <UNITY_database> [<UNITY_where_clause>]\n");
	exit(1);
    }

    /* Get -t option */
    Set_option(OPTS, 'I', argc, argv, &I_opt);
    Set_option(OPTS, 't', argc, argv, &t_opt);
    Set_option(OPTS, 'i', argc, argv, &i_opt);
    Set_option(OPTS, 'C', argc, argv, &C_opt);
    Set_option(OPTS, 'A', argc, argv, &A_opt);
    Set_option(OPTS, 'M', argc, argv, &M_opt);

    /* Reset arguments */
    Clear_opts(OPTS, &argc, &argv);

    /* Parse input */
    in_ptr = 0;
    endofanames = 0;
    where_ptr = argc;
    for (i = 0; i < argc; i++) {
	if (strcmp(argv[i], "in") == 0) {
	    in_ptr = i + 1;
	    endofanames = i;
	}
	if (strcmp(argv[i], "where") == 0)
	    where_ptr = i;
    }

    /* Open table */
    utfp1 = Fopen(argv[in_ptr], "r");

    /* Load D  file */
    if ((nr_attrs_in_dfile = mkntbl(prog, argv[in_ptr], Dtable1, dfile_info, I_opt)) == ERR)
	Quit("Dfile format error", 0);

    if (dfile_info[nr_attrs_in_dfile - 1].flag == WN)
	endf1 = 1;

    /* All attributes from Dfile */
    if (endofanames == 0) {
	for (i = 0; i < nr_attrs_in_dfile; i++) {
	    todo[i].href = NULL;
	    todo[i].th = "";
	    todo[i].td = "";
	    todo[i].templ = NULL;
	    todo[i].attr = i;
	}
	nr_todo = nr_attrs_in_dfile;
    } else {
	for (i = 0; i < endofanames; i++) {

	    /* Template */
	    todo[i].templ = Strsrch(argv[i], "^");
	    if (todo[i].templ != NULL) {
		*(todo[i].templ) = '\0';
		todo[i].templ = todo[i].templ + 1;
	    }
	    /* HREF */
	    todo[i].href = Strsrch(argv[i], "@");
	    if (todo[i].href != NULL) {
		*(todo[i].href) = '\0';
		todo[i].href = todo[i].href + 1;
	    }
	    /* TD */
	    todo[i].td = Strsrch(argv[i], "!");
	    if (todo[i].td != NULL) {
		*(todo[i].td) = '\0';
		todo[i].td = todo[i].td + 1;
	    } else {
		todo[i].td = "";
	    }

	    /* TH */
	    todo[i].th = Strsrch(argv[i], ":");
	    if (todo[i].th != NULL) {
		*(todo[i].th) = '\0';
		todo[i].th = todo[i].th + 1;
	    } else {
		todo[i].th = "";
	    }

	    /* Error checking */
	    if (todo[i].href != NULL && todo[i].templ != NULL)
		Quit("Cannot have Template and Href_spec", 0);

	    /* Find attribute */
	    if ((todo[i].attr = setnum(dfile_info, argv[i], nr_attrs_in_dfile)) == ERR) {
		error(E_ILLATTR, prog, argv[i], Dtable1);
		Quit(0);
	    }
	}
	nr_todo = endofanames;
    }

    /* Process Where */
    p = &argv[where_ptr];
    getcond(p, dfile_info, nr_attrs_in_dfile, prog, Dtable1);
    recordnr = 0;

    /* Print Table header */
    fprintf(stdout, "<TABLE %s>\n", t_opt);
    fprintf(stdout, "<TR>");
    for (i = 0; i < nr_todo; i++) {
	fprintf(stdout, "<TH %s>%s</TH>", todo[i].th, dfile_info[todo[i].attr].aname);
    }
    fprintf(stdout, "</TR>\n");

    /* Go through it */
    for (;;) {
	newrec();
	for (i = 0; i < nr_attrs_in_dfile && getrec(utfp1, &dfile_info[i]) != ERR; i++);
	if (i < nr_attrs_in_dfile)
	    break;
	if (endf1)
	    while (!feof(utfp1) && getc(utfp1) != '\n');
	recordnr++;
	if (selct(dfile_info, recordnr)) {
	    fprintf(stdout, "<TR>");
	    for (i = 0; i < nr_todo; i++) {
		if (todo[i].href != NULL) {
		    fprintf(stdout, "<TD %s>", todo[i].td);
		    href_prt(todo[i].href, dfile_info[todo[i].attr].val);
		    fprintf(stdout, "</TD>");
		} else {
		    if (todo[i].templ != NULL) {
			fprintf(stdout, "<TD %s>", todo[i].td);
			templ_prt(todo[i].templ, dfile_info[todo[i].attr].val);
			fprintf(stdout, "</TD>");
		    } else {
			fprintf(stdout, "<TD %s>%s</TD>", todo[i].td, dfile_info[todo[i].attr].val);
		    }
		}
	    }
	    fprintf(stdout, "</TR>\n");
	}
    }
    /* Print Table header */
    fprintf(stdout, "</TABLE>\n");
    return (0);
}



