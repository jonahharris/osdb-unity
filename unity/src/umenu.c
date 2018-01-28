/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*	@(#)umenu.c	1.8 */
/* the PATH variable must be set to include the unity commands */
/* the next line is name of file containing list of data base files
   it must be in the form
     data_base_file_name|user_file_name
   where any data appearing after the | will be what the user sees
   for the file name instead of the real file name (it is optional).
   dbfiles must be in the current directory when executing.
   if any line has a shell metacharacter in it, it will be expanded
   by the shell first.
*/
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	__STDC__
#include <string.h>
#endif
#include "db.h"

#ifndef	__STDC__
extern char *strcpy(), *strncpy(), *strcat(), *strrchr();
static char *strtok();
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *malloc();
#endif
extern char *getenv();
extern void exit(), free();

static struct {
	char name[MAXPATH+1];
	char uname[MAXUNAME+1];
	} names[MAXTBL];
extern char Uunames[MAXATT][MAXUNAME+1];
extern FILE *get2file();
extern FILE *popen();

static char *prog;
static int filecnt;
static struct fmt xx[MAXATT];
static jmp_buf Quickout;	/* return point when user hits break */
static int nattr1;
static char Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
static char *etargv[4];

static RETSIGTYPE
intr(unused)
int unused;
{
	signal(SIGINT, intr);	/* reset */
	longjmp(Quickout, 0);
}

static RETSIGTYPE
cleanup(unused)
int unused;
{
	et(2,etargv);
	exit(0);
}

main(argc,argv)
int argc;
char *argv[];
{
	char *myargv[512], *myargv3[10];
	char input[1024], *value;
	char num[10], *cond, *relop, *sep, *tok, *sortatt;
	char file[MAXPATH+1+4], ufile[MAXUNAME+1], *attrib;
	int firstpass = 1, pid, verbose, first, flag, i, flag1, noval;
	int nomore, myargc, whereptr, type;
	char tmp1[20];

	if ((prog = strrchr(argv[0],'/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	pid = getpid();
	verbose = 0;
	if(argc != 1) {
		error(E_GENERAL,"No arguments allowed for %s\n",prog);
		exit(1);
	}

	if(rddbfiles() < 0)
		exit(1);

	fprintf(stderr,"Welcome to the unity menu system.\n");

	sprintf(tmp1,"/tmp/umenu1%d",pid);

	myargv[0] = "print";
	myargv[1] = "-b";
	myargv[2] = "-v";
	myargv[3] = "-n~";
	myargc = 4;
	
	flag = 1;
	while(flag) {
		fprintf(stderr,
"You may select an overview of the sequence of operations (type o),\n");
		fprintf(stderr,
"or an expanded instruction option (type e),\n");
		fprintf(stderr,
"or a normal prompt option (type n) or quit (type q).\n");
		fprintf(stderr,
"Option = ");
	        if(gets(input) == NULL)
			exit(1);
	        switch(input[0]) {
		case 'e':
		case 'E':
	                verbose=1;
			flag = 0;
	                break;
	        case 'o':
		case 'O':
                	catover();
	                break;
	        case 'q':
		case 'Q':
	                exit(0);
	        case 'n':
		case 'N':
			flag = 0;
	                verbose=0;
	                break;
	        default:
	                fprintf(stderr,"%s:  Invalid response\n",prog);
	                break;
	        }
	}
	
	/* set arguments for removing temporary files */
	etargv[0] = "et";
	etargv[1] = tmp1;
	etargv[2] = NULL;

	if(signal(SIGINT,intr) == SIG_IGN)
		signal(SIGINT,SIG_IGN);
	if(signal(SIGQUIT,cleanup) == SIG_IGN)
		signal(SIGINT,SIG_IGN);
	if(signal(SIGHUP,cleanup) == SIG_IGN)
		signal(SIGINT,SIG_IGN);
	if(signal(SIGTERM,cleanup) == SIG_IGN)
		signal(SIGINT,SIG_IGN);

	/* Main loop */
	for(;;) {
		setjmp(Quickout);		/* jump back to this point */
		et(2,etargv);
		if(firstpass)
			firstpass = 0;
		else {
			fprintf(stderr,"Do you wish to continue? (y or n) = ");
			input[0] = '\0';
			gets(input);
			if(input[0] != 'y' && input[0] !='Y') {
				exit(0);
			}
		}
		/* set to remove temporaries and exit on interrupt */

		/* get data base file name
		   set "file" to file name, "ufile" to user file name */
		file[0] = ufile[0] = '\0';
		whereptr = 0;
		first=1;
		for(;;) {
			if(verbose) {
				if(first) {
					fprintf(stderr,
"Select your area of interest from the following list of database file names\n");
					first=0;
					strcpy(input,"?");
				}
				else {
					fprintf(stderr,
"Enter the number of Database file you selected = ");
					gets(input);
				}
			}
			else {
				fprintf(stderr,
"Enter number corresponding to Database file name or ? = ");
				gets(input);
			}
			if(strcmp(input,"q") == 0 || strcmp(input,"q") == 0)
				exit(0);
			if(input[0] < '0' || input[0] > '9' ||
				(i=atoi(input)) < 1 || i > filecnt) {
				fprintf(stderr,
				"Valid Database file names:\n");
				for(i=0; i < filecnt; i++) {
					sprintf(num,"%d",i+1);
					fprintf(stderr,
						"     %2.2s %-30.30s",
						num, names[i].uname);
					if((i%2) != 0)
						fprintf(stderr,"\n");
				}
				fprintf(stderr,"\n");
				continue;
			}
			strcpy(ufile,names[i-1].uname);
			strcpy(file,names[i-1].name);
			break;
		}
		if((nattr1 = mkntbl2(prog,file,Dtable1,xx,NULL)) == ERR)
			continue;

	        noval=1;
	        while(noval) {
	                noval=0;
			/* do projection */
			first=1;
			myargc = 4;
			for(;;) {
				if(verbose) {
					if(first) {
						fprintf(stderr,
"A list of characteristics will be displayed which describe the database\n");
						fprintf(stderr,
"file you have chosen.  Select the characteristic(s) which will tell you\n");
						fprintf(stderr,
"what you want to know about this database file.\n");
						first=0;
						strcpy(input,"?");
					}
					else {
						fprintf(stderr,
"Enter number(s) of characteristic(s) you chose separated by commas\n");
						fprintf(stderr,
"or spaces.  If you want to see all of them press Return.\n");
						input[0] = '\0';
						gets(input);
					}
				}
				else {
					fprintf(stderr,
"Enter number(s) of characteristic(s) to be printed,\n");
					fprintf(stderr,
"RETURN for all characteristics printed, or ?. (Separate multiple\n");
					fprintf(stderr,
"characteristics with a single space or comma)\n");
					input[0] = '\0';
					gets(input);
				}
				if(strcmp(input,"q") == 0 ||
					strcmp(input,"q") == 0)
					exit(0);
				if(strcmp(input,"?") == 0) {
					input[0] = '\0';
					fprintf(stderr,
"Valid characteristics for database file %s are:\n",ufile);
					for(i=0; i < nattr1; i++) {
						sprintf(num,"%d",i+1);
						fprintf(stderr,
"   %2.2s %-30.30s",num,Uunames[i]);
						if((i%2) == 1)
							fprintf(stderr,"\n");
					}
					if((i%2) == 1)
						fprintf(stderr,"\n");
					continue;
				}
				break;
			}
			if(input[0] != '\0') {
				for(i=0; input[i]; i++) {
					if(input[i] == ',')
						input[i] = ' ';
				}
				type = myargc;
				sep=" \t";
				attrib = input;
				while((tok=strtok(attrib,sep)) != NULL) {
					attrib = NULL;
					if(tok[0] < '0' || tok[0] > '9' ||
						(i=atoi(tok)) < 1 ||
						i > nattr1) {
						fprintf(stderr,
"Invalid option: %s ignored\n",tok);
						continue;
					}
					myargv[myargc] = xx[i-1].aname;
					myargc++;
				}
				if(myargc > type)
					myargv[myargc] = "in"; myargc++;
			}
	                /* otherwise, all attributes are printed */
	        }

		myargv[myargc] = tmp1; myargc++;

	        flag1=1;
	        relop = NULL;

		/* loop getting values for selection criteria */
	        while(flag1) {
	                flag1=0;
	                input[0] = '\0';
	                nomore=0;
	                first=1;
			/* get attribute name for selection criterion */
	                for(;;) {
	                        if(verbose) {
	                                if(first) {
						fprintf(stderr,
"You have a choice of displaying only those records in your database file\n");
						fprintf(stderr,
"which have specific characteristics you choose.  Each characteristic you\n");
						fprintf(stderr,
"choose is compared to a specific value chosen by you to select the desired\n");
						fprintf(stderr,
"records from the database file.  Choose the characteristic you want to use\n");
						fprintf(stderr,
"in selecting which records are displayed from the following list.\n");
	                                        first=0;
	                                        strcpy(input,"?");
					}
	                                else {
						fprintf(stderr,
"Enter number of characteristic you want to use in selecting which records\n");
						fprintf(stderr,
"are displayed.  If you do not want to choose characteristics, press\n");
						fprintf(stderr,
"Return and your records will be displayed.\n");
	                                        gets(input);
	                                }
				}
	                        else {
					fprintf(stderr,
"Enter number of characteristic for condition if selection criteria\n");
					fprintf(stderr,
"needed, or ? or press RETURN if no selection criteria\n");
	                                gets(input);
	                        }
	                        if(strcmp(input,"q") == 0 ||
	                           strcmp(input,"Q") == 0)
					exit(0);
	                        if(input[0] == '\0') {
	                                nomore=1;
	                                break;
				}
				if(input[0] >= '0' && input[0] <= '9' &&
					(i=atoi(input)) > 0 &&
					i <= nattr1) {
					attrib = xx[i-1].aname;
					break;
				}
				fprintf(stderr,
"Valid characteristics for database file %s:\n",ufile);
				for(i=0; i < nattr1; i++) {
					sprintf(num,"%d",i+1);
					fprintf(stderr,
"   %2.2s %-30.30s",num,Uunames[i]);
					if((i%2) == 1)
						fprintf(stderr,"\n");
				}
				if((i%2) == 1)
					fprintf(stderr,"\n");
	                }
	                if(nomore)
	                        break;

			/* get boolean condition for comparison */
			first=1;
			for(;;) {
				input[0] = '\0';
				if(verbose) {
					if(first) {
						fprintf(stderr,
"You have a choice of ways of making this comparison such as\n");
						fprintf(stderr,
"less than, equal to or greater than a numeric value, and lexically\n");
						fprintf(stderr,
"equal to a word.  Select the comparison method you want to use\n");
						fprintf(stderr,
"from the following list.\n");
						first=0;
						strcpy(input,"?");
					}
					else {
						fprintf(stderr,
"Enter the number of the comparison method you chose = ");
						gets(input);
					}
				}
				else {
					fprintf(stderr,
"Enter number for comparison operator or ? = ");
					gets(input);
				}
				if(input[0] == 'q' || input[0] == 'Q')
					exit(0);
				if(input[0] < '0' || input[0] > '9' ||
					(i=atoi(input)) < 0 || i > 14) {
					fprintf(stderr,
" 1 numerically equals          2 numeric less than\n");
					fprintf(stderr,
" 3 numeric greater than        4 numeric < or equal to\n");
					fprintf(stderr,
" 5 numeric > or equal to       6 numeric not equal to\n");
					fprintf(stderr,
" 7 lexically equal to          8 lex less than\n");
					fprintf(stderr,
" 9 lex greater than           10 lex < or equal to\n");
					fprintf(stderr,
"11 lex > or equal to          12 lex not equal to\n");
					fprintf(stderr,
"13 reg expr equal to          14 reg expr not equal to\n");
					continue;
				}
				switch(i) {
				case 1:
					cond = "eq";
					break;
				case 2:
					cond = "lt";
					break;
				case 3:
					cond = "gt";
					break;
				case 4:
					cond = "le";
					break;
				case 5:
					cond = "ge";
					break;
				case 6:
					cond = "ne";
					break;
				case 7:
					cond = "leq";
					break;
				case 8:
					cond = "llt";
					break;
				case 9:
					cond = "lgt";
					break;
				case 10:
					cond = "lle";
					break;
				case 11:
					cond = "lge";
					break;
				case 12:
					cond = "lne";
					break;
				case 13:
					cond = "req";
					break;
				case 14:
					cond = "rne";
					break;
				}
				break;
			}
	                first=1;
			/* get value for specified attribute */
	                for(;;) {
	                        if(verbose) {
					if(first) {
						first = 0;
						fprintf(stderr,
"Now choose the value of the characteristic used in the comparison.  If\n");
						fprintf(stderr,
"you don't know how the values are specified press \"?\" and a list of\n");
						fprintf(stderr,
"the values in the database file will be displayed.\n");
					}
					fprintf(stderr,
"Enter the value for %s you select for the comparison.\n", attrib);
				}
	                        else {
					fprintf(stderr,
"Enter value for %s in condition or ?\n",attrib);
				}
	                        gets(input);
				if(strcmp(input,"q") == 0 ||
				   strcmp(input,"Q") == 0)
					cleanup(NULL);

				/* list out possible values */
	                        if(strcmp(input,"?") == 0) {
					sprintf(input,
					"uselect -q %s from %s | sort -u > %s",
						attrib, file, tmp1);
					if(system(input) < 0) {
						fprintf(stderr,
"Selection of possible values failed\n");
						unlink(tmp1);
						continue;
					}
					if(!fsizep(tmp1)) {
						fprintf(stderr,
"No values exist for %s in data base %s.\n", attrib, ufile);
	                                        first=1;
	                                        value=NULL;
	                                        break;
	                                }
					else {
						sprintf(input,"tail %s >&2",
							tmp1);
						if(system(input) < 0)
							fprintf(stderr,
"Printing of possible values failed\n");
					}
	                        	unlink(tmp1);
					continue;
				}
	                        if(input[0] == '\0' &&
					(strcmp(cond,"req") == 0 ||
					 strcmp(cond,"rne") == 0)) {
						fprintf(stderr,
"You must have a value if conditions 13 or 14 are selected\n");
	                                        continue;
                                }
				if((value=malloc(strlen(input)+2)) == NULL) {
					fprintf(stderr,
"Cannot allocate space\n");
					exit(1);
				}
				strcpy(value,input);
				break;
	                }
			if(whereptr == 0) {
				whereptr = myargc;
				myargv[myargc] = "where"; myargc++;
			}
	
			if(relop) {
				myargv[myargc] = relop; myargc++;
			}
			myargv[myargc] = attrib; myargc++;
			myargv[myargc] = cond; myargc++;
			myargv[myargc] = value; myargc++;

	                relop=NULL;
	                first=1;

			/* get relational operator */
	                for(;;) {
	                        if(verbose) {
	                                if(first) {
	                                        first=0;
	                                        strcpy(input,"?");
					}
	                                else
	                                        gets(input);
				}
	                        else {
					fprintf(stderr,
"Enter relational operator, RETURN if no more conditions, or ?\n");
	                                gets(input);
	                        }
	                        if(strcmp(input,"q") == 0 ||
	                           strcmp(input,"Q") == 0)
	                                exit(0);
	                        if(input[0] == '\0')
	                                break;
	                        if(strcmp(input,"and") == 0) {
	                                flag1=1;
					relop = "and";
	                                break;
				}
	                        if(strcmp(input,"or") == 0) {
	                                flag1=1;
					relop = "or";
	                                break;
				}
                                if(verbose) {
					fprintf(stderr,
"If you want to use additional characteristics to select which records are\n");
					fprintf(stderr,
"displayed, you may choose to display only records which meet all the\n");
					fprintf(stderr,
"conditions you choose (\"and\") or those which meet any one of the\n");
					fprintf(stderr,
"conditions (\"or\").  Enter \"and\" or \"or\" (without quotes).\n");
					fprintf(stderr,
"If you do not want to use any additional characteristics press Return\n");
					fprintf(stderr,
"and your records will be displayed.\n");
				}
                                else {
					fprintf(stderr,
"If selection criteria are to include multiple conditions and all\n");
					fprintf(stderr,
"conditions must be true, enter an \"and\" (without quotes).\n");
					fprintf(stderr,
"If selection criteria are to include multiple conditions and any of the\n");
					fprintf(stderr,
"conditions are true, enter an \"or\" (without quotes).\n");
					fprintf(stderr,
"If multiple conditions are not needed in selection criteria, press RETURN.\n");
                                }
                                relop = NULL;
	                }
	        }
		myargv[myargc] = NULL;

		/* get sort attribute */
		sortatt=NULL;
		input[0] = '\0';
		first=1;
		for(;;) {
			if(verbose) {
				if(first) {
					fprintf(stderr,
"The characteristics for the file will be displayed.  Select the\n");
					fprintf(stderr,
"characteristic by which you want the selected data sorted.\n");
					first=0;
					strcpy(input,"?");
				}
				else {
					fprintf(stderr,
"Enter number of characteristic on which to sort or press RETURN\n");
					fprintf(stderr,
"if no sort is desired\n");
					gets(input);
				}
			}
			else {
				fprintf(stderr,
"Enter number of characteristic on which to sort or ? for list of\n");
				fprintf(stderr,
"sort attributes or press RETURN if no sort is desired\n");
				gets(input);
			}
			if(strcmp(input,"q") == 0 ||
			   strcmp(input,"Q") == 0)
				exit(0);

			if(input[0] == '\0')
				break;

			if(input[0] >='0' && input[0] <= '9' &&
				(i=atoi(input)) >=1 && i <= nattr1) {
				sortatt = xx[i-1].aname;
				break;
			}
			fprintf(stderr,
"Valid characteristics for database file %s:\n",ufile);
			for(i=0; i < nattr1; i++) {
				sprintf(num,"%d",i+1);
				fprintf(stderr,
"   %2.2s %-30.30s",num,Uunames[i]);
				if((i%2) == 1)
					fprintf(stderr,"\n");
			}
			if((i%2) == 1)
				fprintf(stderr,"\n");
		}

		/* get listing type */
		type=0;
		first=1;
		for(;;) {
			if(verbose) {
				if(first) {
					fprintf(stderr,
"There are two different types of displaying the data.\n");
					fprintf(stderr,
"table - the data is printed in a tabular format with characteristic\n");
					fprintf(stderr,
"        names appearing at the top of each column.\n");
					fprintf(stderr,
"packet - the data is printed horizontally with the characteristic\n");
					fprintf(stderr,
"        names appearing at the left and the data immediately following\n");
					fprintf(stderr,
"        at the right\n");
					fprintf(stderr,
"Enter t for table format or p for packet format\n");
				}
			}
			else
				fprintf(stderr,"Listing type or ?\n");
			input[0] = '\0';
			gets(input);
			if(strcmp(input,"q") == 0 || strcmp(input,"Q") == 0)
				exit(0);
			if(input[0] == 'p') {
				type = 2;	/* catalog command */
				break;
			}
			if(input[0] == 't') {	/* print command */
				type=1;
				break;
			}
			fprintf(stderr,
"Enter t for table format or p for packet format\n");
		}
		if(sortatt) {
			myargv3[0] = "asort"; myargv3[1] = "-q";
			myargv3[2] = sortatt; myargv3[3] = "in";
			myargv3[4] = file; myargv3[5] = "into";
			myargv3[6] = tmp1; myargv3[7] = NULL;
			asort(7, myargv3);
			uclean();
		}
		else {
			myargv3[0] = "uselect"; myargv3[1] = "-q";
			myargv3[2] = "from"; myargv3[3] = file;
			myargv3[4] = "into"; myargv3[5] = tmp1;
			myargv3[6] = NULL;
			uselect(6, myargv3);
			uclean();
		}
	
		if(!fsizep(tmp1))
			fprintf(stderr,
				"No records selected from %s.\n",ufile);
		else {
			if(type == 1) {
				myargv[0] = "uprint";
				myargv[2] = "-v";
				uprint(myargc,myargv);
				uclean();
			}
			else {
				myargv[0] = "catalog";
				myargv[2] = "-n~";/* default but need some
						     argument to fill position*/
				catalog(myargc,myargv);
				uclean();
			}
			fprintf(stderr,"\n\n\n");
		}

		if(whereptr != 0) {
			for(i=whereptr + 3; i < myargc; i += 4)
				free(myargv[i]);
		}
	}
}

static int
catover()
{
	FILE *utfp1;
	char file[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	int c, status;

	if((utfp1 = fopen(OVERVIEW, "r")) == NULL) {
		utfp1=get2file(2, "", OVERVIEW, file, "r");
	}
	if(utfp1 == NULL) {
		error(E_GENERAL, "%s: overview not available\n",prog);
		return(-1);
	}
	status = 0;
	while ((c = getc(utfp1)) != EOF)
		if (putc(c, stderr) == EOF && ferror (stderr)) {
			error(E_GENERAL, "%s: output error\n",prog);
			status = ERR;
			break;
		}
	fclose(utfp1);
	return(status);
}

rddbfiles()
{
	char file[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char buffer[1024];
	FILE *utfp1, *utfp2;
	int l;

	if((utfp1 = fopen(DBFILES, "r")) == NULL)
		utfp1=get2file(2, "", DBFILES, file, "r");

	if(utfp1 == NULL) {
		error(E_GENERAL, "%s: cannot open %s\n",prog,DBFILES);
		return(-1);
	}

	/* read in dbfiles file */
	filecnt = 0;
	while(fgets(&buffer[6],sizeof(buffer) - 6,utfp1) != NULL) {
		strncpy(buffer,"echo \"",6);
		if(shscan(&buffer[6])) {
			l = strlen(buffer);
			buffer[l-1] = '"';	/* in place of newline */
			/* expand with shell */
			if((utfp2=popen(buffer, "r")) == NULL) {
				/* command failed - use unexpanded string */
				buffer[l-1] = '\n';
				get(&buffer[6]);
			}
			else {
				while(fgets(buffer, sizeof(buffer), utfp2) !=
					NULL)
					get(buffer);
				pclose(utfp2);
			}
		}
		else {
			get(&buffer[6]);
		}
	}
	if(filecnt == 0) {
		error(E_GENERAL, "%s: No files listed in %s\n",prog,DBFILES);
		return(-1);
	}
	return(1);
}

static int
shscan(cp)
register char *cp;
{
	for(; *cp; cp++)
		if(any(*cp, "~{[*?$`'\\"))
			return(1);
	return(0);
}

static int
any(c, s)
int c;
register char *s;
{
	register int x;

	while (x = *s++)
		if (x == c)
			return(1);
	return(0);
}

static int
get(buffer)
char *buffer;
{
	int i, c;

	if(filecnt >= MAXTBL)
		return;

	for(i=0;(c = *buffer++)!='|'&& c != '\n' && c!= '\0';i++){
		if(i < MAXPATH)
			names[filecnt].name[i]=c;
	}
	if(i < MAXPATH)
		names[filecnt].name[i] = '\0';
	else
		names[filecnt].name[MAXPATH] = '\0';

	if(c=='\n' || c == '\0') {	/* no user friendly name */
		names[filecnt].uname[0] = '\0';
		filecnt++;
		return;
	}

	for(i=0;(c = *buffer++) != '\n' && c!='\0';i++) {
		if(i < MAXUNAME)
			names[filecnt].uname[i]=c;
	}
	if(i < MAXUNAME)
		names[filecnt].uname[i] = '\0';
	else
		names[filecnt].uname[MAXUNAME] = '\0';
	filecnt++;
}

/* taken from the shell "test" - returns true if file has more than 0 bytes */
static int
fsizep(f)
char	*f;
{
	struct stat statb;

	if(stat(f, &statb) < 0)
		return(0);
	return(statb.st_size > 0);
}

#ifndef	__STDC__
/* compatible with strtok() of System V - some systems don't have it */

static char *
strtok(string, sepset)
char	*string, *sepset;
{
	register char	*p, *q, *r;
	static char	*savept;

	/* first or subsequent call */
	p = (string == NULL)? savept: string;

	if(p == (char *)NULL)	/* return if no tokens remaining */
		return((char *)NULL);

	/* skip leading separators */
	for(q = p; *q != '\0'; ++q) {
		for(r=sepset; *r != '\0' && *r != *q; ++r)
			;
		if(*r == '\0')
			break;
	}

	if(*q == '\0')		/* return if no tokens remaining */
		return((char *)NULL);

	/* move past token */
	r = q;
	do {
		for(p=sepset; *p != '\0' && *p != *r; ++p)
			;
		if(*p != '\0') {	/* found a separator */
			*r = '\0';
			savept = ++r;
			return(q);
		}
		r++;
	} while(*r);

	savept = (char *)NULL;	/* indicate this is last token */
	return(q);
}

#endif
