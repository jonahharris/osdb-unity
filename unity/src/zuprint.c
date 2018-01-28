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
#include <stdio.h>
#include "db.h"

#define COLW 12		/* default column width for terminator fields */
#define WIDTH 79	/* default page width */
#define BOTTOM 4	/* default blank lines at bottom of page */
#define LNOMAX 9999	/* line number max - based on print width */
#define NUMLEN 5	/* length of line number plus space */
#define EFPRINTF fprintf(utfp2,"     ")	/* table shift due to enumeration */

extern char Uunames[MAXATT][MAXUNAME+1];	/* in mktbl2.c */
extern  char    DtmpItable[];

static	char	sep1, sep2, sep3;
static	int	offset, lineno, voption;
static char	newline = '\0';
static char	cheaders = '\0';	/* center column headers */
static int	boption=0;
static int	top = 2;		/* blanks lines at top when loption == 0 */
static int	after_pghdr = 2;	/* blanks lines after page header when loption == 0 */
static int	gap = 2;		/* blanks lines between sections when loption == 0 */
static int	bottom = 0;		/* blanks lines after footer when loption == 0 */
static int	onlyNsections = 0;	/* limit output to only print N sections */
static int	in1space = 0;		/* indent each field by one space Left or Right */

extern	int	end_of_tbl;
extern  FILE    *getdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();
extern	char	*getenv();
extern	char	*strchr();
extern	char	*malloc(), *strrchr();
extern FILE	*utfp1, *utfp2;
extern	char	*stdbuf, *table2;
extern	char	*packedsuffix;

uprint(argc,argv)
int argc;
char **argv;
{
	int	exitcode, poption=0, pageoflw = 0, neednewpage, newpage, first, Roption=0;
	struct	fmt xx[MAXATT];
	int	f_width[MAXATT + 1], endf1=0, curline, lines, needlines;
	int	i, j, width = WIDTH, remains, nattr1, asn[MAXATT],foption=0;
	int	loption=0, dspace=0, toption=0,hoption=0,roption=0;
	int	aoption=0,soption=0,inptr,whereptr,nr,recordnr,curpage;
	int	cescapes, enumrecno, packed, pr0width, sections;
	int	intoptr, onto=0, cont;
	char	*prog, Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*p, *q;				/* generic pointers */
	char	*h_array[MAXATT], *Ioption;
	char	*attrline,*ptr[MAXATT],just[MAXATT],*headline,**pwhere,*footline;
	char	*unity_prt,head_buf[1024],foot_buf[1024];

	int	start, end;	/* first and last attribute printed on a pass */
	struct	fmt *pt;	/* general purpose pointer to a attribute */
	int	l_width;	/* width of current section */
	char	*field_print();	/* function to print fields */
	char	savebuf[MAXREC];/* for option to not print duplicate fields
				   on left */
	char	*saveptr[MAXATT], *s, *t, *tblname;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	table2 = NULL;
	exitcode = 1;
	enumrecno = packed = 0;
	voption = offset = 0;
	lines = 66;
	sep1 = '|';
	sep2 = '-';
	sep3 = ' ';
	Ioption = NULL;
	DtmpItable[0] = '\0';
	cheaders = '\0';
	after_pghdr = -1;
	footline = NULL;
	headline = NULL;
	top = -1;
	gap = -1;
	bottom = -1;
	onlyNsections = 0;
	pr0width = 0;
	in1space = 0;

	unity_prt = getenv("UNITY_PRT");

	while (  argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
		case 'b':
		case 'e':
		case 'p':
		case 't':
		case 'v':
		case 'r':
		case 'R':
			for (i=1; argv[1][i]; i++)
				switch (argv[1][i]) {
				case 'e':
					/* line enumeration */
					lineno = 1;
					if (argv[1][i+1] == '#') {
						++i;
						enumrecno = 1;
					}
					break;
				case 'v':
					/* verbose - print user friendly names */
					voption = 1;
					break;
				case 'p':
					/* page - don't split records across page boundaries */
					poption = 1;
					break;
				case 'r':
					/* no error reporting */
					roption = 1;
					break;
				case 'R':
					/* don't print repeating fields on left */
					Roption = 1;
					break;
				case 'b':
					/* break at last alphanumeric on multi-line field */
					boption = 1;
					break;
				case 't':
					/* dont print header, footer or fill lines */
					toption = 1;
					break;
				default:
					if(!roption)
						error(E_BADFLAG,prog,&argv[1][i]);
					return(exitcode);
				}
			break;
		case 'n':
			/* break field values at newline */
			newline = (argv[1][2] != '\0') ? argv[1][2] : NLCHAR;
			break;
		case 'I':
			Ioption = &argv[1][2];
			break;
		case 's':
			/* separate columns by character */
			sep1 = (argv[1][2] != '\0') ? argv[1][2] : '|';
			soption = 1;
			break;
		case 'd':
			/* double space output */
			dspace = 1;
			sep3 = (argv[1][2] != '\0') ? argv[1][2] : ' ';
			break;
		case 'o':
			/* offset left margin */
			if (argv[1][2] != '\0') {
				if (isdigit(argv[1][2])) {
					p = &argv[1][2];
				} else {
					if(!roption)
						error(E_GENERAL,
						"%s: Unknown offset\n", prog );
					offset = 0;
					p = NULL;
				}
			} else if ((argc > 2) && (isdigit(argv[2][0]))) {
				p = argv[2];
			} else {
				p = NULL;
			}
			if (p != NULL )
			{
				register int i;

				i = *p++ - '0';

				while ( isdigit( *p ) ) {
					i = i * 10 + *p++ - '0';
				}
				if ((*p == '\0') && ( i >= 0 )) {
					if (argv[1][2] == '\0') {
						argc--;
						argv++;
					}
					offset = i;
				} else {
					offset = 0;
				}
			}
			break;
		case 'w':
			/* change page width */
			if (argv[1][2] != '\0') {
				if (isdigit(argv[1][2])) {
					p = &argv[1][2];
				} else {
					if(!roption)
						error(E_GENERAL,
						"%s: Unknown width\n", prog );
					width = WIDTH;
					p = NULL;
				}
			} else if ((argc > 2) && (isdigit(argv[2][0]))) {
				p = argv[2];
			} else {
				width = 120;	/* default for -w */
				p = NULL;
			}
			if (p != NULL )
			{
				register int i;

				i = *p++ - '0';

				while (isdigit(*p)) {
					i = i * 10 + *p++ - '0';
				}
				if (*p == '-') {
					if (isdigit(p[1])) {
						onlyNsections = atoi(p+1);
						if (onlyNsections < 0)
							onlyNsections = 1;
					} else {
						onlyNsections = 1;
					}
				}
				if (((*p == '\0') || (*p == '-')) && ( i >= 0 )) {
					if (argv[1][2] == '\0') {
						argc--;
						argv++;
					}
					if (i < 3 ) {
						width = WIDTH;	/* default width */
					} else {
						width = i;
					}
				} else {
					width = WIDTH;		/* default width */
				}
			}
			break;
		case 'l':
			/* change lines per page */
			loption=1;	/* fill last page with newlines */
			if (argv[1][2] != '\0') {
				if (isdigit(argv[1][2])) {
					p = &argv[1][2];
				} else {
					if(!roption)
						error(E_GENERAL,
						 "%s: Unknown line count\n",
						 prog );
					return(exitcode);
				}
			} else if ((argc > 2) && (isdigit(argv[2][0]))) {
				p = argv[2];
			} else {
				lines = 66;	/* default for -l */
				p = NULL;
			}
			if (p != NULL )
			{
				register int i;

				i = *p++ - '0';

				while ( isdigit( *p ) ) {
					i = i * 10 + *p++ - '0';
				}
				if ((*p == '\0') && ( i >= 0 )) {
					if (argv[1][2] == '\0') {
						argc--;
						argv++;
					}
					if ( i < 10 ) {
						lines = 66;	/* default for -l */
					} else {
						lines = i;
					}
				} else {
					lines = 66;	/* default for -l */
				}
			}
			break;
		case 'u':
			/* dont print box */
			sep2 = ' ';
			if(!soption)
				sep1 = ' ';
			break;
		case 'h':
			/* header given as next argument */
			argc--;
			argv++;
			headline = argv[1];
			hoption = 1;
			break;
		case 'a':
			/* attribute line given as next argument */
			argc--;
			argv++;
			attrline = argv[1];
			aoption = 1;
			break;
		case 'f':
			/* footer line given as next argument */
			argc--;
			argv++;
			footline = argv[1];
			foption = 1;
			break;
		default:
			if(!roption)
				error(E_BADFLAG,prog,argv[1]);
			return(exitcode);
		}
		argc--;
		argv++;
	}

	inptr = whereptr = 0;
	for(i=1;i < argc; i++)
		if(strcmp(argv[i],"in") == 0) {
			inptr = i;
			break;
		}
	for(i=1,intoptr = 0;i < argc; i++) /* find "into" if there is one*/
		if(strcmp(argv[i],"into") == 0 ||
		   strcmp(argv[i],"onto") == 0) {
			intoptr = i;
			if(argv[i][0] == 'o')
				onto = 1;
			break;
		}
	for(i=1;i < argc; i++)
		if(strcmp(argv[i],"where") == 0) {
			whereptr = i;
			break;
		}
	if(!( (inptr == 0 && ((intoptr == 0 && ((whereptr == 0 && argc == 2) ||
						(whereptr == 2 && argc >= 6)))
			||  (intoptr == 2 &&    ((whereptr ==0 && argc == 4) ||
						(whereptr == 4 && argc >= 8)))))
	|| (inptr > 1 && ((intoptr == 0 && ((whereptr == 0 && inptr == argc-2)
						|| (whereptr == inptr + 2)))
			|| (intoptr > 0 && ((whereptr==0 && intoptr == inptr+2)
			|| (whereptr==inptr+4 && intoptr==inptr+2)))))))
	{
		error(E_GENERAL,
"Usage: %s [options] [ aname ... in ] table1 [into table2] \\\n\t[where clause]\n",prog);
		return(exitcode);
	}

	if(whereptr == 0)	/* make sure whereptr set ok for getcond() */
		if(intoptr == 0)
			whereptr = inptr + 2;
		else
			whereptr = inptr + 4;

	if(strcmp(argv[inptr+1],"-") == 0) {
		if(!Ioption) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption) == NULL) {
				if(!roption)
					error(E_GENERAL,
						"%s: no description file specified\n",
						prog);
				return(exitcode);
			}
		}
		utfp1 = stdin;
		onlyNsections = 1;
	}
	else {
		if ( ( utfp1 = fopen( argv[inptr+1], "r" )) == NULL )
		{
			if ((utfp1 = packedopen(argv[inptr+1])) == NULL) {
				if(!roption)
					error(E_DATFOPEN, prog, argv[inptr+1]);
				return(exitcode);
			}
			++packed;	/* reading packed table */
		}
	}
	if ((nattr1 = mkntbl2(prog,argv[inptr+1],Dtable1,xx,Ioption)) == ERR ) {
		if (packed)
		{
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if (xx[nattr1-1].flag == WN) endf1 = 1;

	if (DtmpItable[0]) {
		(void) unlink(DtmpItable);	/* created by getdescrwtbl() */
		DtmpItable[0] = '\0';
	}

	if (unity_prt != NULL) {
		int cescapes, value, top_save, head_save, bottom_save, gap_save;

		cescapes = 0;
		top_save = -1;
		head_save = -1;
		bottom_save = -1;
		gap_save = -1;

		p = unity_prt;

		while ((p != NULL) && ((q = strchr(p, '=')) != NULL)) {

			i = q++ - p;

			switch (i) {
			case 3 :
				if ((isdigit(*q)) && (strncmp(p, "top", i) == 0)) {
					value = atoi(q);
					if (((value != 0) || (*q == '0')) &&
					    (value <= 8)) {
						top_save = value;
					}
				}
				break;
			case 4 :
				if ((isdigit(*q)) && (strncmp(p, "head", i) == 0)) {
					value = atoi(q);
					if (((value != 0) || (*q == '0')) &&
					    (value <= 4)) {
						/* one NL needed for end of header string */
						head_save = value + 1;
					}
				}
				break;
			case 6 :
				if ((isdigit(*q)) && (strncmp(p, "bottom", i) == 0)) {
					value = atoi(q);
					if (((value != 0) || (*q == '0')) &&
					    (value <= 8)) {
						bottom_save = value;
					}
				}
				break;
			case 7 :
				if ((isdigit(*q)) && (strncmp(p, "msecgap", i) == 0)) {
					value = atoi(q);
					if (((value != 0) || (*q == '0')) &&
					    (value <= 4)) {
						gap_save = value;
					}
				}
				break;
			case 8 :
				if (strncmp(p, "cheaders", i) == 0) {
					if ((*q == 'y') || (*q == 'Y')) {
						cheaders = 'c';
					} else {
						cheaders = '\0';
					}
				} else if (strncmp(p, "cescapes", i) == 0) {
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
				} else if (strncmp(p, "in1space", i) == 0) {
					switch (*q) {
					case 'L':
					case 'l':
						in1space = 'L';
						break;
					case 'R':
					case 'r':
						in1space = 'R';
						break;
					case 'Y':
					case 'y':
						in1space = 'Y';
						break;
					case 'N':
					case 'n':
						in1space = 'N';
						break;
					default:
						in1space = '\0';
						break;
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
		if (cescapes) {
			if ((headline) && (strlen(headline) < sizeof(head_buf))) {
				(void)cnvtbslsh(head_buf,headline);
				headline = head_buf;
			}
			if ((footline) && (strlen(footline) < sizeof(foot_buf))) {
				(void)cnvtbslsh(foot_buf,footline);
				footline = foot_buf;
			}
		}
		if ((loption == 0) && (poption == 0) && (toption == 0)) {
			top = top_save;
			after_pghdr = head_save;
			gap = gap_save;
			bottom = bottom_save;
		}
	}

	if (inptr == 0) {	 /* default to print all attributes in table */
		for ( nr = i = 0; i < nattr1; i++ ) {
			pt = &xx[ i ];
			/* check if attribute is to be printed by default */
			if ( ! isupper( pt->justify ) ) {
				if ( ( pt->prnt > 0 ) || ( pr0width != 'N' ) ) {
					asn[nr] = i;
					f_width[nr] = 0;
					++nr;
				}
			}
		}
		if ( nr == 0 ) {
			/* select all attributes to be printed */
			for ( i = 0; i < nattr1; i++ ) {
				asn[i] = i;
				f_width[i] = 0;
			}
			nr = nattr1;
		}
	}
	else {	 /* Print only selected attributes */
		for ( nr = i = 0; i < inptr-1; i++ ) {
			if (nr >= MAXATT) {
				if(!roption)
					error(E_GENERAL,
					"%s: Too many attributes specified.\n",
					prog);
				if (packed)
				{
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			f_width[nr] = 0;
			just[nr] = '\0';
			for(j=0;argv[i+1][j] != '\0'; j++) {
			/* look for print width override */
				if(argv[i+1][j] == ':')
					break;
			}
			if (argv[i+1][j] == ':')
			{		/* get print width and/or justification */
				int	prnt;
				char	justify;

				argv[i+1][j++] = '\0';
				prnt = -1;
				justify = '\0';
				while ( argv[i+1][j] == ' ' ) {
					++j;	/* skip over leading white space */
				}
				switch ( argv[i+1][j] ) {
				case 'c':
				case 'C':
				case 'l':
				case 'L':
				case 'r':
				case 'R':
					justify = argv[i+1][j++];
					break;
				case 'n':
					if ( ( strncmp( &argv[i+1][j], "nodisplay=", 10 ) == 0 ) &&
					     ( strcmp( argv[i+1], "all" ) == 0 ) )
					{
						/*
						 * Restore the ':' so lookup on attr "all" below
						 * will fail and then we can check for special
						 * attribute "all" before reporting an error.
						 */
						argv[i+1][3] = ':';
					}
					break;
				default:
					break;
				}
				if ( isdigit( argv[i+1][j] ) )
				{
					prnt = argv[i+1][j++] - '0';
					while ( isdigit( argv[i+1][j] ) ) {
						prnt = prnt * 10 + argv[i+1][j++] - '0';
					}
					switch ( argv[i+1][j] ) {
					case 'c':
					case 'C':
					case 'l':
					case 'L':
					case 'r':
					case 'R':
						if ( justify == '\0' )
							justify = argv[i+1][j++];
						break;
					default:
						break;
					}
				}
				while ( argv[i+1][j] == ' ' ) {
					++j;	/* skip over trailing white space */
				}
				if ( argv[i+1][j] == '\0' ) {
					if ( prnt >= 0 )
						f_width[nr] = prnt;
					if ( justify != '\0' )
						just[nr] = tolower( justify );
				}
			}
			if ( (asn[nr] = setnum( xx, argv[i+1], nattr1)) != ERR)
			{
				++nr;
			}
			else if (( strncmp( argv[i+1], "all", 3 ) == 0 ) &&
				   (( argv[i+1][3] == '\0' ) || ( argv[i+1][3] == ':' )))
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
				if ( ( argv[i+1][3] == ':' ) &&
				     ( argv[i+1][13] == '=' ) &&
				     ( argv[i+1][14] != '\0' ) )
				{
					p = &argv[i+1][14];
					while ( ( p != NULL ) && ( *p != '\0' ) ) {
						q = strchr( p, ',' );
						if ( q != NULL ) {
							*q = '\0';
						}
						if ((j = setnum( xx, p, nattr1)) == ERR) {
							if(!roption)
								error(E_ILLATTR,prog,argv[i+1],Dtable1);
							if (packed)
							{
								/* EOF == FALSE */
								(void)packedclose(utfp1,FALSE);
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
						if (nr >= MAXATT) {
							if(!roption)
								error(E_GENERAL,
								"%s: Too many attributes specified.\n",
								prog);
							if (packed)
							{
								/* EOF == FALSE */
								(void)packedclose(utfp1,FALSE);
								utfp1 = NULL;
							}
							return(exitcode);
						}
						asn[nr] = j;
						f_width[nr] = 0;
						just[nr] = '\0';
						++nr;
					}
				}
			}
			else
			{
				if(!roption)
					error(E_ILLATTR,prog,argv[i+1],Dtable1);
				if (packed)
				{
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
		}
	}
	if(intoptr == 0) {
		utfp2 = stdout;
		if((stdbuf=malloc(BUFSIZ)) != NULL)
			setbuf(stdout,stdbuf);
		tblname = "-";		/* current output table name */
	}
	else {
		if(strcmp(argv[intoptr+1],"-") != 0) {
			if(chkaccess(argv[intoptr+1],00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,argv[intoptr+1]);
					if (packed)
					{
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
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
				if (packed)
				{
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
		}
		else
			utfp2 = stdout;

		tblname = argv[intoptr+1];	/* current output table name */
	}

	if(toption)
		loption = 0;	/* no pagination if -t option */
	if(!loption)
		poption = 0;	/* if no pagination, no paging of recs needed */
	if(lineno)
		width = width - NUMLEN;	/* reduce for line enumeration */

	/* Determine the field width for each attribute to be printed. */
	for ( i = 0; i < nr; i++ ) {
		pt = &xx[ asn[i] ];

		if (f_width[i] > 0)
			;
		else if ( pt->prnt > 0 ) /* field width provided in desc file */
			f_width[i] = pt->prnt;
		else if ( pt->flag == WN ) {	/* fixed width field */
			if (voption)
				f_width[i] = strlen ( Uunames[asn[i]] );
			else
				f_width[i] = strlen ( pt->aname );
			if ( f_width[i] < pt->flen )
				f_width[i] = pt->flen;
		}
		else	/* terminator field */
			f_width[i] = COLW;	/* default width */

		if ( f_width[i] > width - 2 )	/* check if too wide */
			f_width[i] = width -2;
		switch (just[i]) {
		case 'l':
		case 'r':
		case 'c':
			break;
		case 'L':
		case 'R':
		case 'C':
			just[i] = tolower(just[i]);
			break;
		default:
			just[i] = tolower(pt->justify);
			break;
		}
	}

	/* parse where clause and build evaluation tree */
	pwhere = &argv[whereptr];
	getcond(pwhere,xx,nattr1,prog,Dtable1);


	/* Until all the attributes have been printed
	   for as many attributes as can be printed in a section
			print heading
			print fields
	*/

	curpage = 0;
	curline = 0;
	sections = 0;

	f_width[nr] = width; /* to stop inner for loop */
	for(start = 0; start < nr; start = end) {

		/* find number of attributes that will fit in section */
		l_width = f_width[start] + 2;

		for(end=start+1; l_width+f_width[end]+1 <= width; end++ )
			l_width += f_width[end] + 1;

		++sections;	/* increment section count */
		recordnr = 0;	/* first record for this section */
		first = 1;	/* first time through this section */
		newpage = 0;	/* didn't just start a new page */
		if(Roption) {
			savebuf[0] = '\0';	/* null string */
			for(i=start; i< end; i++)
				saveptr[i] = savebuf;
		}
		/* for entire table, read a tuple and print attributes */
		end_of_tbl = 0;
		for(;;) {
			/* get a record */
			newrec();
			for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
			if (( i == 0 ) && ( end_of_tbl ))
				break;
			else if ( i < nattr1 )
			{
				error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
					prog, i, recordnr + 1, argv[inptr+1] );
				if (packed)
				{
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return( exitcode );
			}
			if(endf1)
				if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
					error( E_GENERAL,
						"%s: Error: missing newline on rec# %d in %s\n",
						prog, recordnr + 1, argv[inptr+1] );
					if (packed)
					{
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				} else if (i != '\n') {
					error( E_GENERAL,
						"%s: Error: data overrun on rec# %d in %s\n",
						prog, recordnr + 1, argv[inptr+1] );
					if (packed)
					{
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}

			recordnr++;
			if(!selct(xx,recordnr)) {
				lineno += enumrecno;	/* enumrecno is either 0 or 1 */
				continue;
			}

			for(i=start; i< end; i++)
				ptr[i] = xx[asn[i]].val;
			if(Roption) {	/* null out duplicates on left */
				for(i=start; i< end; i++) {
					if(strcmp(ptr[i],saveptr[i]) == 0)
						ptr[i] = "";
					else
						break;
				}
				if(i == end)/* all duplicates - don't print */
					continue;

				/* save for next record comparison */
				if(i == start)
					s = savebuf;
				else
					s = saveptr[i-1] + strlen(saveptr[i-1])
						+ 1;
				for(; i< end; i++) {
					saveptr[i] = s;
					for(t=xx[asn[i]].val; *s++ = *t++; );
				}
			}

			remains = 1;
			cont = 0;	/* record continuation flag */
			while(remains) {
				remains = 0;
				if(!toption && first)
					neednewpage = 1;
				else
					neednewpage = 0;
				if(loption && lines - curline <= BOTTOM)
					neednewpage = 1;

				if(poption && !neednewpage && !cont &&
					!newpage) {
				/* if the page split option is on,
				   a new page is not already needed,
				   this is not already a new page,
				   and this is the first line of a new record
				   to be printed (!cont), determine if the
				   entire record will fit on the page -
				   set neednewpage if record won't fit */
					needlines = 1;
					for(i = start;i < end;i++) {
						j = fp2(f_width[i], ptr[i],
							newline, boption);
						if(j > needlines)
							needlines = j;
					}
					if(curline+needlines >= lines - BOTTOM)
						neednewpage = 1;
				}
				if(neednewpage && newpage) {
					/* make sure at least one line printed
					   on each page */
					neednewpage = 0;
				}
				if(Roption && (neednewpage || pageoflw) &&
					! cont )
				{
					/* when beginning new page,
					   print all attributes */
					for(i=start; i< end; i++)
						ptr[i] = xx[asn[i]].val;
					pageoflw = 0;
				}
				else if ( Roption && neednewpage && cont )
				{
					pageoflw = 1;
				}

				if(neednewpage) {
					/* begin new page */
					if(!first) {
						/* finish current section */
						separator(l_width - 2);
						curline++;
					}
					if(!first && loption) {
						if(foption) {
							for(;curline <
								lines-BOTTOM+1;
								curline++)
								putc('\n',
									utfp2);
							for(i=0;i<offset;i++)
								putc(' ',utfp2);
							fprintf(utfp2,footline,
								curpage);
						}
						/* move to new page */
						while(curline++ < lines)
							putc('\n',utfp2);
					}

					curline = 0;
					if ((first) && (sections == 1) && (top >= 0) &&
					   ((gap >= 0) || (onlyNsections == 1) || (end >= nr))) {
						for (i=0; i < top; i++) {
							putc('\n',utfp2);
							++curline;
						}
					} else if ((first) && (sections >= 2) && (gap >= 0)) {
						for (i=0; i < gap; i++) {
							putc('\n',utfp2);
							++curline;
						}
					} else {
						/* print default two line header */
						fprintf(utfp2,"\n\n");
						curline = 2;
					}
					curpage++;
					if(hoption) {
						for(i=0;i<offset;i++)
							putc(' ',utfp2);
						fprintf(utfp2,headline,curpage);
						if ((first) && (sections == 1) && (after_pghdr >= 0) &&
						   ((gap >= 0) || (onlyNsections == 1) || (end >= nr))) {
							for (i=0; i <= after_pghdr; i++) {
								putc('\n',utfp2);
								++curline;
							}
						} else {
							fprintf(utfp2,"\n\n\n");
							curline += 3;
						}
					}
					if(aoption) {
						for(i=0;i<offset;i++)
							putc(' ',utfp2);
						fprintf(utfp2,attrline,curpage);
						fprintf(utfp2,"\n");
						separator(l_width - 2);
						curline += 2;
					}
					else {
						for(j = start;j < end; j++ )
							if(voption)
						h_array[j] = Uunames[asn[j]];
							else
						h_array[j] = xx[asn[j]].aname;

						curline += head_print(l_width,
							start, end, f_width,
							h_array, just);
					}
					newpage = 1;
				}
				else if(toption && first && start > 0) {
					/* print 2 newlines between sections */
					fprintf(utfp2,"\n\n");
				}

				/* print the next line of output */
				for(i=0;i<offset;i++)
					putc(' ',utfp2);
				if(lineno) {
					if(!cont) {
						if (lineno <= LNOMAX)
							fprintf(utfp2,"%4d ", (lineno % (LNOMAX+1)) );
						else
							fprintf(utfp2,"%04.4d ", (lineno % (LNOMAX+1)) );
						++lineno;
					}
					else
						EFPRINTF;
				}
				for(i = start;i < end;i++) {
					putc(sep1, utfp2);
					if((ptr[i] = field_print(f_width[i],
					 ptr[i],just[i],newline, boption, in1space))
						!= NULL )
						remains = 1; /* more data */
				}
				putc(sep1,utfp2);
				putc('\n',utfp2);
				newpage = 0;	/* printed at least 1 line */
				first = 0;	/* not first time through */
				curline++;
				cont = 1;	/* if another line is printed,
						   it is a continuation line */
			}
			/* finished printing current record */
			if(dspace && !(loption && lines - curline <= BOTTOM) ) {
				/* double space option and don't need a new
				   page yet - print extra (blank) line */
				for(i=0;i<offset;i++)
					putc(' ',utfp2);
				if(lineno)
					EFPRINTF;
				putc(sep1,utfp2);
				for(j=start; j < end; j++) {
					for(i=f_width[j];i > 0; i--)
						putc(sep3,utfp2);
					putc(sep1,utfp2);
				}
				putc('\n',utfp2);
				curline++;
			}
		}

		/* finished all records for the current section */
		if(!first) {
			/* finish up the section */
			if(!toption) {
				separator(l_width - 2);
				curline++;
			}
			if(loption) {
				/* handle page end - only if paginating */
				if(foption) {
					for(;curline < lines - BOTTOM + 1;
						curline++)
						putc('\n',utfp2);
					for(i=0;i<offset;i++)
						putc(' ',utfp2);
					fprintf(utfp2,footline,curpage);
				}
				/* move to new page */
				while(curline++ < lines)
					putc('\n',utfp2);
				curline = 0;
			}
		}
		if ((utfp1 == stdin) || (end >= nr) ||
		   ((onlyNsections) && (sections >= onlyNsections))) {
			if ((bottom >= 0) && (curpage) &&
			   ((sections == 1) || (gap >= 0))) {
				if ((footline != NULL) && (foption == 0)) {
					fprintf(utfp2,footline,curpage);
					putc('\n',utfp2);
					++curline;
				}
				for (i = 0; i < bottom; i++) {
					putc('\n',utfp2);
					++curline;
				}
			}
			break;	/* nothing else can be done */
		}
		if (packed) {
			/* EOF == TRUE */
			if ((packedclose(utfp1,TRUE) != 0) ||
			    ((utfp1 = packedopen(argv[inptr+1])) == NULL)) {
				error(E_PACKREAD,prog,argv[inptr+1],packedsuffix);
				utfp1 = NULL;
				return(exitcode);
			}
		}
		else
			rewind(utfp1);	/* in case more attributes to print */
		if(lineno)
			lineno = 1;
	}

	if (utfp1 != stdin) {
		if (packed) {
			/* EOF == TRUE */
			if (packedclose(utfp1,TRUE) != 0) {
				error(E_PACKREAD,prog,argv[inptr+1],packedsuffix);
				utfp1 = NULL;
				return(exitcode);
			}
		}
		else
			fclose(utfp1);
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
	exitcode = 0;
	return(exitcode);
}

static int
head_print( l_width, start, end, f_width, h_array, just)
int	l_width;	/* line width for current section */
int	start;		/* starting attribute for section */
int	end;		/* ending attribute for section   */
int	f_width[];	/* widths for all fields	  */
char	*h_array[];	/* array of pointers to attribute names */
char	just[];		/* attribute name justification */
{
	int	remains, i, linecnt;
	char *field_print();


	separator(l_width - 2);
	linecnt = 1;		/* number of lines printed */


	/* remains is a boolean variable used to insure
	   that the entire field is printed, no matter
	   how many lines it takes  */
	remains = 1;
	while ( remains ) {	/* print heading for section */
		remains = 0;
		for(i=0;i<offset;i++)
			putc(' ',utfp2);
		if(lineno)
			EFPRINTF;
		for ( i = start; i < end; i++ ) {
			putc(sep1,utfp2);
			if ( ( h_array[i] = field_print( f_width[i], h_array[i],
				cheaders ? 'c' : just[i], newline , boption, in1space )) !=NULL)
				remains = 1;
		}
		putc(sep1,utfp2);
		putc('\n',utfp2);
		linecnt++;
	}

	separator(l_width - 2);
	return(++linecnt);
}

separator(w)
int w;
{
	int i;
	/* separator line */
	for(i=0;i<offset;i++) putc(' ',utfp2);
	if(lineno)
		EFPRINTF;
	putc(sep1,utfp2);
	for ( i = 0; i < w; i++ )
		putc( sep2 ,utfp2);
	putc(sep1,utfp2);
	putc( '\n',utfp2);
}


static char *
field_print( width, start, justify, new_line, brkalpha, in1space )
int	width;		/* maximum field width */
char	*start;		/* beginning of field to print */
char	justify;	/* positioning character */
char	new_line;	/* newline character or NULL */
int	brkalpha;	/* break at non-alphanumeric ? */
{
	/* field_print prints width characters from the string "start"
	   on stdout.  If field_print reaches the end of "start" before
	   printing width characters it pads with blanks.  If field_print
	   does not find the NULL character it returns a pointer to the
	   next character in "start".  If field value too long, break at
	   new_line character or non-alphanumeric if specified.  */

	char *end;
	int length, l2, found;

	if ( start == NULL ) {	/* blank field */
		while ( width-- )
			putc( ' ' ,utfp2);
		return NULL;
	}

	length = strlen (start);
	found = 0;
	if (new_line) {
		l2 = length;
		if(length > width)
			length = width;

		/* look for embedded newline character */
		for(end=start; end < start + length && *end != new_line &&
			*end != '\n';end++);

		if(*end == new_line || *end == '\n')
			found++;
		else
			length = l2;
	}
	if (!found) {
		if (length > width)
			/* look for good break point */
			if (brkalpha  &&  *(start+width) != ' ') {
				for(end=start+width-1;
		    			end>start+1 &&
					*end != ' ' &&
					*end != ',' &&
					*end != ';' &&
					*end != ':'; end--);
				if (end == start+1)
					for(end=start+width-1;
						end>start+1 && isalnum(*end);
						end--);
				if(end == start+1)
					end = start + width;
				else
					end++;
			}
			else
				end = start + width;
		else
			end = start + length;
	}

	/* handle the justification */
	length = width - (end - start); /* number of spaces to be printed */
	switch ( justify ) {
	case 'r':
		if ( ( length >= 1 ) && ( width >= 3 ) &&
		     ( ( in1space == 'R' ) || ( in1space == 'Y' ) ) ) {
			length -= 1;
			while ( length-- )
				putc( ' ' ,utfp2);
			length = 1;
		} else {
			while ( length-- )
				putc( ' ' ,utfp2);
		}
		break;
	case 'c':
		for ( length -= length / 2; length; length-- ) {
			width--;
			putc( ' ' ,utfp2);
		}
		length = width - (end - start);
		break;
	case 'l':
	default:
		if ( ( length >= 1 ) && ( width >= 3 ) &&
		     ( ( in1space == 'L' ) || ( in1space == 'Y' ) ) ) {
			putc( ' ' ,utfp2);
			length -= 1;
		}
		break;
	}

	/* print the field */
	while ( start != end ) {
		/*
		 * Tabs can screw up the alignment,
		 * so convert them to spaces.
		 */
		if ( *start != '\t' )
			putc( *start, utfp2);
		else
			putc( ' ', utfp2);
		++start;
	}
	/* and pad to the end with spaces.  */
	while ( length-- > 0 )
		putc( ' ',utfp2);

	while ( *end == ' ' )
		end = end + 1;

	if ( *end == NULL )
		return NULL;		/* return NULL if at end of string */
	if ( *end == new_line  || *end == '\n') {
		if(*(end+1) == NULL)
			return NULL;
		return
			end + 1;	/* return next character to print */
	}
	return end;
}

static int
fp2(width, start, new_line ,brkalpha)
int	width;		/* maximum field width */
register char	*start;	/* beginning of field to print */
char	new_line;	/* newline character or NULL */
int	brkalpha;	/* break at non-alphanumeric ? */
{
	/* fp2 is a modified version of field_print which just returns
	   the number of lines need to print the attribute value */

	register char *end;
	int length, l2, found, l;

	l = 0;
	while(start && *start) {
		length = strlen(start);
		found = 0;
		if(new_line) {
			l2 = length;
			if(length > width)
				length = width;

			/* look for embedded newline character */
			for(end=start; end < start + length && *end!=new_line &&
				*end != '\n';end++);

			if(*end == new_line || *end == '\n')
				found++;
			else
				length = l2;
		}
		if(!found) {
			if (length > width) {
				/* look for good break point */
				if (brkalpha  &&  *(start+width) != ' ') {
					for(end=start+width-1;
			    			end>start+1 &&
						*end != ' ' &&
						*end != ',' &&
						*end != ';' &&
						*end != ':'; end--);
					if (end == start+1)
						for(end=start+width-1;
							end>start+1 &&
							isalnum(*end);
							end--);
					if(end == start+1)
						end = start + width;
					else
						end++;
				}
				else
					end = start + width;
			}
			else
				end = start + length;
		}
		l++;

		while( *end == ' ' )
			end = end + 1;

		if(*end == NULL)
			start = NULL;
		else if(*end == new_line || *end == '\n') {
			if(*(end+1) == NULL)
				start = NULL;
			else
				start = end + 1;
		}
		else
			start = end;
	}
	if(l == 0)
		return(1);
	return(l);
}

print(argc,argv)
int argc;
char **argv;
{
	return(uprint(argc,argv));
}

char *
cnvtbschar( src, dest )
char *src;
char *dest;
{
	short i;
	short cnt;

	switch( *src ) {
	case '\0':
		*dest = '\\';
		break;	/* don't increment src past null terminator */
	case 'n':
		*dest = '\n';
		src++;
		break;
	case 't':
		*dest = '\t';
		src++;
		break;
	case 'a':
		*dest = '\a';
		src++;
		break;
	case 'b':
		*dest = '\b';
		src++;
		break;
	case 'r':
		*dest = '\r';
		src++;
		break;
	case 'f':
		*dest = '\f';
		src++;
		break;
	case 'v':
		*dest = '\v';
		src++;
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		i = 0;	/* octal number */
		cnt = 1;
		do {
			i = 8 * i + *src++ - '0';
		} while ( isdigit( *src ) && *src < '8' && cnt++ < 3 );
		*dest = (char) i;
		break;
	default:
		*dest = *src++;
		break;
	}

	return( src );
}

cnvtbslsh( startdest, src )
char *startdest;
register char *src;
{
	register char *dest;

	for( dest = startdest; *src; dest++ ) {
		if ( *src == '\\' )
			src = cnvtbschar( src + 1, dest );
		else
			*dest = *src++;
	}

	*dest = '\0';	/* null terminate the string */

	return( dest - startdest );
}
