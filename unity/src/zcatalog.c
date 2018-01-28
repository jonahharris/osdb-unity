/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include "db.h"

/* defaults */
#define WIDTH 79	/* width of printout */
#define MARGIN 35	/* width of margin for printing attributes */
			/* only up to MAXUNAME characters can be printed
			   in margin, no matter how big margin is set */

static struct	fmt xx[MAXATT];
static char	*enumformat;
extern char Uunames[MAXATT][MAXUNAME+1];	/* in mktbl2.c */
static int nattr1, curline, loption, voption, eoption, enumber, nohyphens;
static int lines, width, margin, offset, enumgap;

extern	char	*table2, Dtable2[], *stdbuf;
extern	char	DtmpItable[];
extern FILE	*utfp1, *utfp2;

extern	int	end_of_tbl;
static	int	exitcode, pgnumber, new;
static char	*footer, *header;
static char	hyphoffch;
extern char	*packedsuffix;

extern	char	*getenv(), *strchr(), *strrchr();
extern	char	*strcpy();
extern	FILE	*getdescrwtbl();
extern	FILE	*packedopen();
extern	int	cnvtbslsh();
extern	int	packedclose();

catalog(argc, argv)
char	*argv[];
int	argc;
{
	char	newline, number[10], *Ioption, *tblname;
	int	endf1, i, blank, printed, onto, doption, lmargin;
	int	inptr,whereptr,intoptr,recordnr,nattr2,pr[MAXATT];
	int	enumrecno, packed, pr0width;
	char	Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*start, *prog, **where, sep[1024];
	char	*unity_prb, enumprbuf[1024],head_buf[1024],foot_buf[1024];

	struct	fmt *pt;	/* general purpose pointer to a attribute */

 	eoption = loption = curline = blank = printed = endf1 = 0;  
	enumrecno = enumgap = offset = nohyphens = packed = voption = 0;
	enumformat = "--- %d ---";
	hyphoffch = ' ';
	onto = doption = lmargin = enumber = pgnumber = 0;
	pr0width = 0;
	width = WIDTH;		/* width of total page */
	margin = MARGIN;	/* width of margin for attribute names */
	lines = 66;		/* not used unless loption set */
	Ioption = NULL;

	stdbuf = table2 = NULL;
	header = footer = NULL;
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	exitcode = 1;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	newline=NLCHAR;
	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0')
	{
		char	*p, *q;

		switch(argv[1][1]) {
			default:
				error(E_GENERAL,"%s: Unknown flag\n",prog);
				error(E_GENERAL, "Usage: %s %s \\\n\t%s \\\n\t%s\n", prog,
					"[-llines] [-mmargin] [-ooffset] [-wwidth] [-n] [-b]",
					"[-e[#]] [-v[!]] [-d] [-Itable] [-h header] [-f footer]",
					"[ aname ... in ] table1 [into table2] [where-clause]");
				return(exitcode);
			case 'b':
			case 'd':
			case 'e':
			case 'v':
				for ( i = 1; argv[1][i]; i++ ) {
					switch ( argv[1][i] ) {
					case 'd':
						doption = 1;
						break;
					case 'b':
						blank = 1;
						break;
					case 'v':
						if (argv[1][2] == '!') {
							voption = -1;
							++i;
						} else {
							voption = 1;
						}
						break;
					case 'e':
						eoption = 1;
						if(argv[1][2] == '#') {
							enumrecno = 1;
							++i;
						}
						break;
					}
				}
				break;
			case 'I':
				Ioption = &argv[1][2];
				break;
			case 'h':
				header = argv[2];
				argc--; argv++;
				break;
			case 'f':
				footer = argv[2];
				argc--; argv++;
				break;
			case 'n':
				newline=argv[1][2];
				break;
			case 'l':
				loption=1;
				if (argv[1][2] != '\0') {
					if (isdigit(argv[1][2])) {
						p = &argv[1][2];
					} else {
						error(E_GENERAL,
						"%s: Unknown line count\n",
						prog);
						return(exitcode);
					}
				} else if ((argc > 2) && (isdigit(argv[2][0]))) {
					p = argv[2];
				} else {
					lines = 66;
					p = NULL;
				}
				if (p != NULL)
				{
					register int i;

					i = *p++ - '0';
					while (isdigit(*p)) {
						i = i * 10 + *p++ - '0';
					}
					if ((*p == '\0') && (i >= 0)) {
						if (argv[1][2] == '\0') {
							argc--;
							argv++;
						}
						if (i < 10) {	/* minimum */
							lines = 66;
						} else {
							lines = i;
						}
					}
				}
				break;
			case 'm':
				if (argv[1][2] != '\0') {
					if ((argv[1][2] == '-' ) &&
					   ((argv[1][3] == '\0') ||
					    (argv[1][3] >= '0' && argv[1][3] <= '9'))) {
						++lmargin;
						if (argv[1][3]) {
							p = &argv[1][3];
						} else {
							p = NULL;
						}
					} else if (isdigit(argv[1][2])) {
						p = &argv[1][2];
					} else {
						error(E_GENERAL,
						"%s: Unknown margin count\n",
						prog);
						return(exitcode);
					}
				} else if ((argc > 2) && (isdigit(argv[2][0]))) {
					p = argv[2];
				} else {
					p = NULL;
				}
				if (p != NULL)
				{
					register int i;

					i = *p++ - '0';
					while (isdigit(*p)) {
						i = i * 10 + *p++ - '0';
					}
					if ((*p == '\0') && (i >= 0)) {
						if (argv[1][2] == '\0') {
							argc--;
							argv++;
						}
						if (i < 10) {
							margin = 10;	/* minimum */
						} else {
							margin = i;
						}
					}
				}
				break;
			case 'o':
				if (argv[1][2] != '\0') {
					if ((argv[1][2] == '-' ) &&
					   ((argv[1][3] == '\0') ||
					    (argv[1][3] >= '0' && argv[1][3] <= '9'))) {
						if (argv[1][3]) {
							hyphoffch = '-';
							p = &argv[1][3];
							offset = atoi(&argv[1][3]);
						} else {
							++nohyphens;
							p = NULL;
						}
					} else if (isdigit(argv[1][2])) {
						hyphoffch = ' ';
						p = &argv[1][2];
					} else {
						error(E_GENERAL,
						"%s: Unknown offset count\n",
						prog);
						return(exitcode);
					}
				} else if ((argc > 2) && (isdigit(argv[2][0]))) {
					hyphoffch = ' ';
					p = argv[2];
				} else {
					p = NULL;
				}
				if (p != NULL)
				{
					register int i;

					i = *p++ - '0';
					while (isdigit(*p)) {
						i = i * 10 + *p++ - '0';
					}
					if ((*p == '\0') && (i >= 0)) {
						if (argv[1][2] == '\0') {
							argc--;
							argv++;
						}
						if (i < 0) {
							offset = 0;
						} else {
							offset = i;
						}
					}
				}
				break;
			case 'w':
				if (argv[1][2] != '\0') {
					if (isdigit(argv[1][2])) {
						p = &argv[1][2];
					} else {
						error(E_GENERAL,
						"%s: Unknown width count\n",
						prog);
						return(exitcode);
					}
				} else if ((argc > 2) && (isdigit(argv[2][0]))) {
					p = argv[2];
				} else {
					p = NULL;
				}
				if (p != NULL)
				{
					register int i;

					i = *p++ - '0';
					while (isdigit(*p)) {
						i = i * 10 + *p++ - '0';
					}
					if ((*p == '\0') && (i >= 0)) {
						if (argv[1][2] == '\0') {
							argc--;
							argv++;
						}
						if (i < 10) {
							width = 10;	/* minimum */
						} else {
							width = i;
						}
					}
				}
				break;
		}
		argc--;
		argv++;
	}
	if(width < margin + 5)
		width = margin + 5;
	for(i=0; i < width && i < sizeof(sep)-1; i++)
		sep[i] = '-';
	sep[i] = '\0';

	inptr = whereptr = intoptr = 0;
	for(i=1;i<argc;i++)
		if(strcmp(argv[i],"in") == 0) {
			inptr = i;
			break;
		}
	for(i=1;i<argc;i++)
		if(strcmp(argv[i],"where") == 0) {
			whereptr = i;
			break;
		}
	for(i=1;i<argc;i++)
		if(strcmp(argv[i],"into") == 0 ||
		   strcmp(argv[i],"onto") == 0) {
			intoptr = i;
			if(argv[i][0] == 'o')
				onto = 1;
			break;
		}
	if(!( (inptr == 0 && ((intoptr == 0 && ((whereptr == 0 && argc == 2) ||
						(whereptr == 2 && argc >= 6)))
			||  (intoptr == 2 &&    ((whereptr ==0 && argc == 4) ||
						(whereptr ==4 && argc >= 8)))))
	|| (inptr > 1 && ((intoptr == 0 && ((whereptr == 0 && inptr == argc-2)
						|| (whereptr == inptr + 2)))
			|| (intoptr > 0 && ((whereptr==0 && intoptr == inptr+2)
			|| (whereptr==inptr+4 && intoptr==inptr+2)))))))
	{
		error(E_GENERAL, "Usage: %s %s \\\n\t%s \\\n\t%s\n", prog,
			"[-llines] [-mmargin] [-ooffset] [-wwidth] [-n] [-b]",
			"[-e[#]] [-v[!]] [-d] [-Itable] [-h header] [-f footer]",
			"[ aname ... in ] table1 [into table2] [where-clause]");
		return(exitcode);
	}

	if(whereptr == 0)
		if(intoptr == 0)
			whereptr = inptr + 2;
		else
			whereptr = inptr + 4;

	if(intoptr != 0 && strcmp(argv[intoptr+1],"-") != 0) {
		if(chkaccess(argv[intoptr+1],00) == 0) {
			if(!onto) {
				error(E_EXISTS,prog,argv[intoptr+1]);
				return(exitcode);
			}
		}
		else
			table2 = argv[intoptr+1];
		if(onto)
			utfp2 = fopen(argv[intoptr+1],"a");
		else
			utfp2 = fopen(argv[intoptr+1],"w");
		if(utfp2 == NULL) { 
			error(E_DATFOPEN,prog,argv[intoptr+1]);
			return(exitcode);
		}
		tblname = argv[intoptr+1];	/* current output table name */
	} else {
		tblname = "-";
		utfp2 = stdout;
	}

	if (strcmp(argv[inptr+1],"-") == 0) {
		if (!Ioption) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption) == NULL) {
				error(E_GENERAL,
					"%s: Description file not specified.\n", prog);
				return(exitcode);
			}
		}
		utfp1 = stdin;
	}
	else {
		if ((utfp1 = fopen(argv[inptr+1],"r")) == NULL ) { 
			if ((utfp1 = packedopen(argv[inptr+1])) == NULL) {
				error(E_DATFOPEN,prog,argv[inptr+1]);
				return(exitcode);
			}
			++packed;	/* reading packed table */
		}
	}

	if((nattr1 = mkntbl2(prog,argv[inptr+1],Dtable1,xx,Ioption))==ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if (xx[nattr1-1].flag == WN) endf1 = 1;

	/* no longer need any temporary input descriptor table */
	if (DtmpItable[0]) {
		/* Note this will also remove Dtable1 */
		unlink(DtmpItable);
		DtmpItable[0] = '\0';
	}

	unity_prb = getenv("UNITY_PRB");

	if (unity_prb != NULL) {
		char	*p, *q;
		int value, alignlm, cescapes, notverbose;

		alignlm = cescapes = notverbose = 0;

		p = unity_prb;

		while ((p != NULL) && ((q = strchr(p, '=')) != NULL)) {

			i = q++ - p;

			switch (i) {
			case 7 :
				if (strncmp(p, "alignlm", i) == 0) {
					if (( *q == 'y' ) || ( *q == 'Y' )) {
						alignlm = 1;
					} else {
						alignlm = 0;
					}
				} else if (strncmp(p, "verbose", i) == 0) {
					if (( *q == 'n' ) || ( *q == 'N' )) {
						notverbose = 1;
					} else {
						notverbose = 0;
					}
				} else if (strncmp(p, "enumfmt", i) == 0) {
					char *commaptr;
					int len;
					commaptr = strchr( q, ',' );
					if ( commaptr ) {
						len = commaptr - q;
					} else {
						len = strlen(q);
					}
					if (( len >= 1 ) && ( len < sizeof(enumprbuf) )) {
						strncpy( enumprbuf, q, len );
						enumprbuf[len] = '\0';
						(void)cnvtbslsh(enumprbuf,enumprbuf);
						enumformat = enumprbuf;
					}
				} else if (strncmp(p, "enumgap", i) == 0) {
					if (isdigit(*q)) {
						value = atoi(q);
						if (((value != 0) || (*q == '0')) &&
						    (value <= 2)) {
							enumgap = value;
						}
					}
				}
				break;
			case 8 :
				if (strncmp(p, "cescapes", i) == 0) {
					if ((*q == 'y') || (*q == 'Y')) {
						++cescapes;
					} else {
						cescapes = 0;
					}
				} else if (strncmp(p, "pr0width", i) == 0) {
					if ((*q == 'n') || (*q == 'N')) {
						pr0width = 'N';
					} else {
						pr0width = 0;
					}
				}
				break;
			default:
				break;
			}
			if ((p = strchr(q, ',')) != NULL) {
				++p;
			}
		}
		if ( alignlm )
			++lmargin;
		if ( ( notverbose ) && ( ! voption ) )
			voption = -1;
		if (cescapes) {
			if ((header) && (strlen(header) < sizeof(head_buf))) {
				(void)cnvtbslsh(head_buf,header);
				header = head_buf;
			}
			if ((footer) && (strlen(footer) < sizeof(foot_buf))) {
				(void)cnvtbslsh(foot_buf,footer);
				footer = foot_buf;
			}
		}
	}

	if (voption < 0) {	/* do not use verbose names */
		for (i=0;i<nattr1; i++)
			strcpy(Uunames[i], xx[i].aname);
	} else if (lmargin) {	/* remove leading white space from left margin */
		for (i=0;i<nattr1; i++) {
			for (lmargin=0; Uunames[i][lmargin] == ' '; lmargin++) ;
			if (lmargin) strcpy(Uunames[i], &Uunames[i][lmargin]);
		}
	}

	if ( inptr > 0 ) {
		for ( i = 1, nattr2 = 0; i < inptr; i++ )
		{
			int	j;

			if ( nattr2 >= MAXATT) {
				error(E_GENERAL,
					"%s: Too many attributes specified.\n", prog);
				if (packed)
				{
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}

			if ( ( j = setnum( xx, argv[i], nattr1 ) ) != ERR ) {
				pr[nattr2] = j;
				++nattr2;
			}
			else if (( strncmp( argv[i], "all", 3 ) == 0 ) &&
				(( argv[i][3] == '\0' ) ||
				 ( strncmp( &argv[i][3], ":nodisplay=", 11 ) == 0 )))
			{
				char	skip[MAXATT+1];

				/*
				 * Request to display all attributes
				 * except for listed after the optional
				 * :nodisplay= keyword.
				 */
				for ( j = 0; j < nattr1; j++ ) {
					skip[j] = 0;
				}
				if ( ( argv[i][3] == ':' ) &&
				     ( argv[i][14] != '\0' ) )
				{
					char	*p, *q;

					p = &argv[i][14];
					while ( ( p != NULL ) && ( *p != '\0' ) ) {
						q = strchr( p, ',' );
						if ( q != NULL ) {
							*q = '\0';
						}
						if ((j = setnum( xx, p, nattr1)) == ERR) {
							error( E_ILLATTR, prog, argv[i], Dtable1 );
							if (packed)
							{
								/* EOF == FALSE */
								(void)packedclose( utfp1, FALSE );
								utfp1 = NULL;
							}
							return(exitcode);
						}
						skip[j] = 1;
						if ( q ) {
							*q++ = ',';
						}
						p = q;
					}
				}
				for ( j = 0; j < nattr1; j++ ) {
					if ( skip[j] == 0 ) {
						if ( nattr2 >= MAXATT ) {
							error( E_GENERAL,
								"%s: Too many attributes specified.\n",
								prog );
							if (packed)
							{
								/* EOF == FALSE */
								(void)packedclose( utfp1, FALSE );
								utfp1 = NULL;
							}
							return(exitcode);
						}
						pr[nattr2] = j;
						++nattr2;
					}
				}
			}
			else
			{
				error( E_ILLATTR, prog, argv[i], Dtable1 );
				if ( packed ) {
					/* EOF == FALSE */
					(void)packedclose( utfp1, FALSE );
					utfp1 = NULL;
				}
				return(exitcode);
			}
		}
	}
	else {
		for ( nattr2 = i = 0; i < nattr1; i++ ) {
			pt = &xx[ i ];
			/* check if attribute is to be printed by default */
			if ( ! isupper( pt->justify ) ) {
				if ( ( pt->prnt > 0 ) || ( pr0width != 'N' ) ) {
					pr[ nattr2 ] = i;
					++nattr2;
				}
			}
		}
		if ( nattr2 == 0 ) {
			/* select all attributes to be printed */
			for ( i=0; i<nattr1; i++ ) {
				pr[i] = i;
			}
			nattr2 = nattr1;
		}
	}

	/* parse where clause and build evaluation tree */
	where = &argv[whereptr];
	getcond(where,xx,nattr1,prog,Dtable1);

	recordnr = end_of_tbl = 0;
	printed = 1;
	for(;;) {
		newrec(); 
		for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if ((i == 0) && (end_of_tbl))
			break;
		else if (i < nattr1) {
			error(E_GENERAL,
				"%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, recordnr + 1, argv[inptr+1]);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		if (endf1) 
			if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
				error( E_GENERAL,
					"%s: Error: missing newline on rec# %d in %s\n",
					prog, recordnr + 1, argv[inptr+1] );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on rec# %d in %s\n",
					prog, recordnr + 1, argv[inptr+1] );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}

		recordnr++;
		if(!selct(xx,recordnr)) {
			enumber += enumrecno;
			continue;
		}

		if (eoption)
			++enumber;

		if(printed) {
			if(loption) {
				fprintf(utfp2,"\n\n\n\n");
				curline = 4;
				if(header) {
					pgnumber++;
					fprintf(utfp2,header,pgnumber);
					fprintf(utfp2,"\n\n");
					curline += 2;
				}
				if(eoption) {
					for(i = offset; i != 0; --i)
						putc(' ', utfp2);
					fprintf(utfp2,enumformat,enumber);
					putc('\n', utfp2);
					curline++;
					for(i = enumgap; i != 0; --i) {
						putc('\n', utfp2);
						curline++;
					}
				}
			} else {
				curline = 0;	/* not used */
				putc('\n', utfp2);
				if(nohyphens) {
					if (eoption) {
						for(i = offset; i != 0; --i)
							putc(' ', utfp2);
						fprintf(utfp2,enumformat,enumber);
						putc('\n', utfp2);
						for(i = enumgap; i != 0; --i) {
							putc('\n', utfp2);
						}
					}
				} else {
					for(i = offset; i != 0; --i)
						putc(hyphoffch, utfp2);
					fprintf(utfp2,"%s\n",sep);
					if(eoption) {
						sprintf(number,"%d",enumber);
						for(i=0; number[i]; i++);
						number[i++] = ' ';
						for(; i < sizeof(number); i++)
							number[i] = '-';
						number[sizeof(number)-1] = '\0';
						if (hyphoffch == '-') {
							fprintf(utfp2,"%s%s",number,
								&sep[sizeof(number)-1]);
							for(i = offset; i != 0; --i)
								putc('-', utfp2);
							putc('\n', utfp2);
							putc('\n', utfp2);
						} else {	/* space char */
							for(i = offset; i != 0; --i)
								putc(' ', utfp2);
							fprintf(utfp2,"%s%s\n\n",number,
								&sep[sizeof(number)-1]);
						}
					}
					else {
						for(i = offset; i != 0; --i)
							putc(hyphoffch, utfp2);
						fprintf(utfp2,"%s\n\n",sep);
					}
				}
			}
			new = 1;
		}
		printed = 0;

		for(i=0;i<nattr2;i++) {
			start=xx[pr[i]].val;
			if(blank && start[0] == '\0')
				continue;

			if(doption && !new)
			{
				putc('\n', utfp2);
				curline++;
			}

			printed++;	/* at least one attribute printed */
			new = 0;

			if (xx[pr[i]].flag==T)
				termprint(Uunames[pr[i]],newline,start,
					newline,utfp2);
			else
				termprint(Uunames[pr[i]],newline,start,
					'\0',utfp2);
		}
		if(loption == 1 && curline > 0 && curline < lines) {
			/* print footer */
			for(; curline < lines - 4; curline++)
				putc('\n',utfp2);
			if(footer) {
				putc('\n', utfp2);
				fprintf(utfp2,footer,pgnumber);
				fprintf(utfp2,"\n\n");
				curline += 3;
			}
			for(;curline < lines;curline++)
				putc('\n', utfp2);
			curline = 0;
		}
	}

	if (utfp1 != stdin) {
		if (packed) {
			/* EOF == TRUE */
			if (packedclose(utfp1,TRUE) != 0) {
				error(E_PACKREAD,prog,argv[inptr+1],packedsuffix);
				utfp1 = NULL;
				return(exitcode);
			}
		} else {
			fclose(utfp1);
		}
	}
	utfp1 = NULL;

	if (fflush(utfp2) == EOF) {
		error(E_DATFWRITE, prog, tblname);
		return(exitcode);
	}
	if (utfp2 != stdout) {
		if (fclose(utfp2) == EOF) {
			error(E_DATFWRITE, prog, tblname);
			return(exitcode);
		}
		utfp2 = NULL;
	}

	table2 = NULL;
	Dtable2[0] = '\0';
	exitcode = 0;
	return(exitcode);
}

static char *
findend(start,newline, valwidth)
char *start;
char newline;
int valwidth;
{
	char *end;
	int length, l2, found;

	if(start == NULL || *start == NULL)
		return(start);

	length = strlen (start);
	found = 0;
	if (newline) {
		l2 = length;
		if(length > valwidth)
			length = valwidth;

		/* look for embedded newline character */
		for(end=start; end < start + length &&
			 *end != newline && *end != '\n';end++);
		if(*end == newline || *end == '\n')
			found++;
		else
			length = l2;
	}
	if (!found) {
		if (length > valwidth) {
			/* look for good break point */
			if(*(start+valwidth) != ' ') {
				for(end=start+valwidth-1;
		    			end>start+1 &&
					*end != ' ' &&
					*end != ',' &&
					*end != ';' &&
					*end != ':'
					;end--);
				if(end == start+1)
					for(end=start+valwidth-1;
						end>start+1 &&
						isalnum(*end);
						end--);
				if(end == start+1)
					end = start + valwidth;
				else
					end++;
			}
			else
				end = start + valwidth;
		}
		else
			end = start + length;
	}
	return(end);
}

static int
termprint(head, headnl, start, newline, file)
char *head;
char headnl;
char *start;
char newline;
FILE *file;
{
	char *endhead, *end;
	int i, trouble;

	trouble = 0;	/* tight loop detector for bug in getrec() */

	end = start;
	endhead = head;

	while(*end || *endhead) {
		start = end;
		head = endhead;
		end = findend(start, newline, width - margin);
		endhead = findend(head, headnl, margin - 1);

		if ((start == end) && (head == endhead) && (trouble++))
			break;		/* break out of tight loop */

		new = 0;

		for(i = offset; i != 0; --i)
			putc(' ', file);
		/* print the header in the margin */
		for(i=0; head != endhead; i++)
			putc(*head++, file);
		for(; i < margin; i++)
			putc(' ', file);

		/* print the field */
		while(start != end)
			putc(*start++,file);
		putc('\n',file);

		curline++;
		if(loption == 1 && curline >= lines - 4) {
			/* print footer */
			if(footer) {
				fprintf(utfp2,"\n");
				fprintf(utfp2,footer,pgnumber);
				fprintf(utfp2,"\n\n");
				curline += 3;
			}
			for(;curline < lines;curline++)
				fprintf(utfp2,"\n");

			curline = 0;

			/* print header */
			fprintf(utfp2,"\n\n\n\n");
			curline = 4;
			if(header) {
				pgnumber++;
				fprintf(utfp2,header,pgnumber);
				fprintf(utfp2,"\n\n");
				curline += 2;
			}
			if(eoption) {
				fprintf(utfp2,enumformat,enumber);
				putc('\n', utfp2);
				curline++;
				for(i = enumgap; i != 0; --i) {
					putc('\n', utfp2);
					curline++;
				}
			}
			new = 1;
		}

		if(*end != '\0') {
			if(*end == newline)
				end++;
			while(*end == ' ')
				end = end + 1;
		}
		if(*endhead != '\0') {
			if(*endhead == headnl)
				endhead++;
			while(*endhead == ' ')
				endhead = endhead + 1;
		}
	}
}
