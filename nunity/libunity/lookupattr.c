/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "message.h"
#include "uquery.h"

extern char *basename();

/*
 * Global variable which must be set to QP_NEWVALUE
 * by the application to indicated that attribute
 * modifier "value=" is allowed (to be recognized).
 */
int	uqpnvalue = 0;

char *
lookupnode( aname, nodelist, nodecnt, nodeptr )
char *aname;
struct qnode *nodelist;
int nodecnt;
struct qnode **nodeptr;
{
	register int i;
	register char *tmp, *ptr;

	/*
	 * Look up relation name in the nodelist;
	 */

	ptr = strrchr( aname, '.' );
	if ( ptr == NULL ) {
		/*
		 * The format was <attrname> only. Check if there is only
		 * one database and if so, set the node to be the only
		 * relation.
		 */
		if ( nodecnt == 1 ) {
			*nodeptr = nodelist;
			return( aname );
		}
		else
			return( NULL );
	}

	*ptr++ = '\0';

	i = 0;
	for( tmp = aname; *tmp && isdigit( *tmp ); )
		i = i * 10 + (*tmp++ - '0');

	if ( *tmp == '\0' ) {
		/*
		 * The format was <number>.<attrname>.
		 * Convert the file number and look up the file.
		 */
		--i;
		if ( i >= 0 && i < nodecnt ) {
			*nodeptr = &nodelist[i];
			return( ptr );
		}
	}
	else {
		/*
		 * The format was <relname>.<attrname>.
		 * Look up the relname in the database descriptions.
		 */
		for( i = 0; i < nodecnt; i++ ) {
			if ( strcmp( basename( nodelist[i].rel->path ), aname) == 0 ) {
				*nodeptr = &nodelist[i];
				return( ptr );
			}
		}
	}

	return( NULL );
}

static
_lookupattr( fullname, nodelist, nodecnt, nodeptr, attrnum, projection )
char *fullname;
struct qnode *nodelist;
int nodecnt;
struct qnode **nodeptr;
short *attrnum;
int projection;
{
	register int i;
	register char *aname;
	/*
	 * MAXPATHLEN here should really be MAXNAMELEN,
	 * but that doesn't seem to be defined in a common
	 * place for all OSs.
	 */
	char attrbuf[MAXPATHLEN + ANAMELEN + 1 + 4];	/* allow for "/./" or "././" prefix */

	/*
	 * Look up attribute in the database descriptions.
	 */
	*attrnum = -1;
	*nodeptr = NULL;

	strcpy( attrbuf, fullname );

	if (  nodecnt > 1 && strchr( fullname, '.' ) == NULL ) {
		/*
		 * We have just an attribute name, but multiple relations
		 * it could come from.  So go through each relation
		 * looking for the attribute.  It must occur one time
		 * and only one time.
		 */
		aname = attrbuf;
		if ( strcmp( aname, "rec#" ) == 0 || strcmp( aname, "seek#" ) == 0 )
		{
			/*
			 * The special attributes "all", "rec#", and
			 * "seek#" are defined for every relation.  They
			 * cannot be found this way.  The check below that
			 * findattr( ) returns an attribute number >= 0
			 * will filter out the special "all" attribute.
			 */
			return( FALSE );
		}

		for( i = 0; i < nodecnt; i++ ) {
			short tmpattr;

			tmpattr = findattr( nodelist[i].rel->attrs,
						nodelist[i].rel->attrcnt,
						aname );
			if ( tmpattr >= 0 ) {
				if ( *nodeptr ) {
					/*
					 * Attribute is multiply defined.
					 * Return no match and let the parser
					 * decide what to do.
					 */
					*attrnum = -1;
					*nodeptr = NULL;
					return( FALSE );
				}
				*nodeptr = &nodelist[i];
				*attrnum = tmpattr;
			}
		}

		/*
		 * If no attribute name matched, we
		 * let the parser report an error, if
		 * really appropriate.
		 */
		return( *attrnum >= 0 );
	}

	aname = lookupnode( attrbuf, nodelist, nodecnt, nodeptr );
	if ( aname == NULL ) {
		/*
		 * No relation name/number matched.
		 * Return and let the parser report an error, if
		 * really appropriate.
		 */
		return( FALSE );
	}

	/*
	 * Check for the special record number attribute.
	 */
	if ( strcmp( aname, "rec#" ) == 0 ) {
		*attrnum = ATTR_RECNUM;
		return( TRUE );
	}
	else if ( strcmp( aname, "seek#" ) == 0 ) {
		*attrnum = ATTR_SEEK;
		return( TRUE );
	}

	/*
	 * Look for the attribute name among the attributes for the
	 * relation.
	 */
	*attrnum = findattr( nodeptr[0]->rel->attrs, nodeptr[0]->rel->attrcnt,
			aname );

	/*
	 * Check if it is the special "all" attribute.
	 */
	if ( *attrnum < 0 && projection && strcmp( aname, "all" ) == 0 ) {
		*attrnum = ATTR_ALL;
		return( TRUE );
	}

	/*
	 * If the attribute name was unrecognized, we
	 * return and let the parser report an error, if
	 * really appropriate.
	 */
	return( *attrnum >= 0 );
}

lookupattr( fullname, nodelist, nodecnt, attrptr )
char *fullname;
struct qnode *nodelist;
int nodecnt;
struct attrref *attrptr;
{
	return( _lookupattr( fullname, nodelist, nodecnt, &attrptr->rel,
				&attrptr->attr, FALSE ) );
}

static void
subsort( projptr, range )
struct qprojection *projptr;
char *range;
{
	int rangenum;
	char *nextptr;
	char *charstr;
	char delim;
	struct qsubsort *sortptr;

	sortptr = projptr->subsort;
	projptr->subcnt = 0;

	while( range && *range != '\0' ) {
		nextptr = strpbrk( range, ",-" );
		if ( nextptr ) {
			delim = *nextptr;
			*nextptr = '\0';
		}
		else
			delim = '\0';

		charstr = strchr( range, '.' );
		if ( charstr )
			*charstr = '\0';

		rangenum = atoi( range ) - 1;
		if ( rangenum > 0 ) {
			if ( projptr->delim == NULL ) {
				/*
				 * We've got a field other than the first.
				 * Make sure there is a delimiter for
				 * field splitting.  This may be later
				 * overridden.
				 */
				projptr->delim = ","; /* default delimiter */
			}
			sortptr->strtfld = rangenum;
		}
		else
			sortptr->strtfld = 0;

		if ( charstr ) {
			rangenum = atoi( charstr + 1 ) - 1;
			sortptr->strtchar = (rangenum > 0) ? rangenum : 0;
			*charstr = '.';
		}
		else
			sortptr->strtchar = 0;	/* all chars */

		if ( delim == ',' || delim == '\0' ) {
			/* no range to search on */
			if ( delim == ',' )
				*nextptr = ',';
			sortptr->endfld = sortptr->strtfld;
			sortptr->endchar = charstr ? sortptr->strtfld + 1 : 0;
		}
		else {
			*nextptr = '-';
			range = nextptr + 1;
			nextptr = strchr( range, ',' );
			if ( nextptr )
				*nextptr = '\0';

			if ( *range == '\0' ) {
				/* go to end of attribute string */
				sortptr->endfld = 0xff;
				sortptr->endchar = 0;
			}
			else {
				charstr = strchr( range, '.' );
				if ( charstr ) {
					*charstr = '\0';

					/*
					 * endchar represents the character
					 * after the last one to copy, so
					 * don't decrement the given value.
					 */
					sortptr->endchar = atoi( charstr + 1 );

					*charstr = '.';
				}
				else {
					sortptr->endchar = 0;	/* all chars */
				}

				sortptr->endfld = atoi( range ) - 1;

				if ( sortptr->endfld < sortptr->strtfld ) {
					sortptr->endfld = sortptr->strtfld;
					sortptr->endchar = sortptr->strtchar + 1;
				}
				else if ( sortptr->endfld == sortptr->strtfld &&
						sortptr->endchar >= 0 &&
						sortptr->endchar <= sortptr->strtchar )
					sortptr->endchar = sortptr->strtchar + 1;
			}

			if ( nextptr )
				*nextptr = ',';
		}

		if ( ++projptr->subcnt == QP_MAXSUBSRT )
			break;
		++sortptr;

		if ( nextptr == NULL )
			break;

		range = nextptr + 1;
	}
}

attr_modifier( projptr, modstr, msgs )
struct qprojection *projptr;
char *modstr;
char msgs;
{
	char *nextstr;
	char *numstr;
	char dflt_sort;
	char good_modifier;
	int rc = TRUE;	/* return code */

	if ( modstr == NULL || *modstr == '\0' )
		return( TRUE );

	dflt_sort =  ( projptr->attr == ATTR_RECNUM ||
				projptr->attr == ATTR_SEEK ) ?
			QP_NUMERIC : QP_STRING;

	while( 1 ) {
		while( *modstr == ':' )
			modstr++;

		nextstr = strchr( modstr, ':' );
		while( nextstr != NULL ) {
			int i;

			i = -1;
			while( modstr < &nextstr[i] && nextstr[i] == '\\' )
				--i;
			if ( ( ( -i ) % 2 ) == 0 )	/* escaped colon */
				nextstr = strchr( nextstr + 1, ':' );
			else {
				*nextstr = '\0';
				break;
			}
		}

		numstr = strpbrk( modstr, "0123456789" );

		good_modifier = TRUE;

		switch( *modstr ) {
		case 'a':
			if ( strncmp( modstr, "asc", 3 ) == 0 ) {
				/* ascending sort on this attribute */
				projptr->flags &= ~QP_DESCENDING;
				projptr->flags |= QP_SORT;
				if ( (projptr->flags & QP_SORTMASK) == 0 )
					projptr->flags |= dflt_sort;
			}
			else
				good_modifier = FALSE;
			break;
		case 'b':
			if ( strncmp( modstr, "bl", 2 ) == 0 ) {
				/* remove initial blanks before sorting */
				projptr->flags |= QP_RMBLANKS;
				if ( (projptr->flags & QP_SORTMASK) == 0 )
					projptr->flags |= dflt_sort;
			} else if ( strncmp( modstr, "bi", 2 ) == 0 ) {
				/* binary number comparison */
				if ( projptr->attr != ATTR_RECNUM &&
					projptr->attr != ATTR_SEEK )
				{
					projptr->flags &= ~QP_SORTMASK;
					projptr->flags |= QP_BINARY;
				}
			}
			else
				good_modifier = FALSE;
			break;
		case 'c':
		case 'C':
		case 'l':
		case 'L':
		case 'r':
		case 'R':
			if ( strncmp( modstr, "ca", 2 ) == 0 ) {
				/* caseless compare */
				if ( projptr->attr != ATTR_RECNUM &&
					projptr->attr != ATTR_SEEK )
				{
					unsigned oldval;
					oldval = projptr->flags & QP_SORTMASK;
					projptr->flags &= ~QP_SORTMASK;
					switch (oldval) {
					case QP_DICTIONARY:
					case QP_NOCASEDICT:
						projptr->flags |= QP_NOCASEDICT;
						break;
					case QP_PRINTABLE:
					case QP_NOCASEPRINT:
						projptr->flags |= QP_NOCASEPRINT;
						break;
					default:
						projptr->flags |= QP_CASELESS;
						break;
					}
				}
			}
			else {  /* justifcation and width */
				projptr->justify = *modstr;
				if ( numstr )
					projptr->prwidth = atoi( numstr );
			}
			break;
		case 'd':
			if ( strncmp( modstr, "des", 3 ) == 0 ) {
				/* descending sort */
				projptr->flags |= QP_DESCENDING;
				projptr->flags |= QP_SORT;
				if ( (projptr->flags & QP_SORTMASK) == 0 )
					projptr->flags |= dflt_sort;
			}
			else if ( strncmp( modstr, "delim", 5 ) == 0 ) {
				/* delimiter char for attr on output */
				/* remove back slashes in delimstr */
				if ( modstr[5] ) {
					cnvtbslsh( &modstr[5], &modstr[5] );
					projptr->terminate = modstr[5];
					projptr->attrwidth = 0;
					projptr->attrtype = QP_TERMCHAR;
				}
			}
			else if ( strncmp( modstr, "da", 2 ) == 0 ) {
				/* date comparison */
				if ( projptr->attr != ATTR_RECNUM &&
					projptr->attr != ATTR_SEEK )
				{
					projptr->flags &= ~QP_SORTMASK;
					projptr->flags |= QP_DATE;
				}
			}
			else if ( strncmp( modstr, "dic", 3 ) == 0 ) {
				/* dictionary sort */
				if ( projptr->attr != ATTR_RECNUM &&
					projptr->attr != ATTR_SEEK )
				{
					unsigned oldval;
					oldval = projptr->flags & QP_SORTMASK;
					projptr->flags &= ~QP_SORTMASK;
					switch (oldval) {
					case QP_CASELESS:
					case QP_NOCASEDICT:
						projptr->flags |= QP_NOCASEDICT;
						break;
					default:
						projptr->flags |= QP_DICTIONARY;
						break;
					}
				}
			}
			else if ( strncmp( modstr, "dis", 3 ) == 0 ) {
				/* display the attribute */
				projptr->flags &= ~QP_NODISPLAY;
				if ( ( projptr->attr != ATTR_ALL ) &&
				     ((projptr->flags & QP_SORTMASK) == 0 ) )
					projptr->flags |= dflt_sort;
			}
			else
				good_modifier = FALSE;
			break;
		case 'f':
			if ( strncmp( modstr, "fld", 3 ) == 0 ||
					strncmp( modstr, "field", 5 ) == 0 ) {
				/* sub-fields to sort on */
				projptr->flags |= QP_SORT;
				if ( numstr )
					subsort( projptr, numstr );
			}
			else
				good_modifier = FALSE;
			break;
		case 'h':
			/* hexadecimal number comparison */
			if ( projptr->attr != ATTR_RECNUM && projptr->attr != ATTR_SEEK )
			{
				projptr->flags &= ~QP_SORTMASK;
				projptr->flags |= QP_HEXADECIMAL;
			}
			break;
		case 'n':
			if ( strncmp( modstr, "nu", 2 ) == 0 ) {
				/* numeric comparison */
				projptr->flags &= ~QP_SORTMASK;
				projptr->flags |= QP_NUMERIC;
			}
			else if ( strncmp( modstr, "noca", 4 ) == 0 ) {
				/* caseless comparison */
				if ( projptr->attr != ATTR_RECNUM &&
					projptr->attr != ATTR_SEEK )
				{
					projptr->flags &= ~QP_SORTMASK;
					projptr->flags |= QP_CASELESS;
				}
			}
			else if ( strncmp( modstr, "nod", 3 ) == 0 ) {
				if ( ( projptr->attr == ATTR_ALL ) &&
				     ( strncmp( modstr, "nodisplay=", 10 ) == 0 ) )
				{
					char	*q;
					register char *p;
					register int	j;
					register struct uattribute *attrptr;
		
					/*
					 * Do not display the given attribute(s)
					 * if/when ATTR_ALL is expanded.
					 */
					if ( modstr[10] != '\0' ) {
						p = &modstr[10];
					} else {
						p = NULL;
					}
					while ( p )
					{
						if ( ( q = strchr( p, ',' ) ) != NULL )
							*q = '\0';
						for ( j = projptr->rel->rel->attrcnt,
						      attrptr = projptr->rel->rel->attrs;
						      j > 0; attrptr++, j-- )
						{
							if ( strcmp( p, attrptr->aname ) == 0 )
								break;
						}
						if ( j <= 0 ) {
							if ( msgs )
								prmsg( MSG_ERROR, "unrecognized nodisplay attribute name '%s'", p );
							if ( q ) *q = ',';
							p = NULL;
							good_modifier = FALSE;
							break;
						}
						if ( q ) {
							*q = ',';
							p = q + 1;
						} else {
							projptr->attorval = &modstr[10];
							p = NULL;
							break;
						}
					}
				} else {
					/* don't display attr */
					projptr->flags |= QP_NODISPLAY;
				}
			}
			else if ( strncmp( modstr, "nov", 3 ) == 0 ) {
				projptr->flags |= QP_NOVERBOSE;
			}
			else
				good_modifier = FALSE;
			break;
		case 'o':
			/* octal number comparison */
			if ( projptr->attr != ATTR_RECNUM && projptr->attr != ATTR_SEEK )
			{
				projptr->flags &= ~QP_SORTMASK;
				projptr->flags |= QP_OCTAL;
			}
			break;
		case 'p':
			if ( strncmp( modstr, "pr", 2 ) == 0 ) {
				/* dictionary sort */
				if ( projptr->attr != ATTR_RECNUM &&
					projptr->attr != ATTR_SEEK )
				{
					unsigned oldval;
					oldval = projptr->flags & QP_SORTMASK;
					projptr->flags &= ~QP_SORTMASK;
					switch (oldval) {
					case QP_CASELESS:
					case QP_NOCASEPRINT:
						projptr->flags |= QP_NOCASEPRINT;
						break;
					default:
						projptr->flags |= QP_PRINTABLE;
						break;
					}
				}
			}
			else
				good_modifier = FALSE;
			break;
		case 's':
			if ( strncmp( modstr, "str", 3 ) == 0 ) {
				/* normal string comparison */
				if ( projptr->attr != ATTR_RECNUM &&
					projptr->attr != ATTR_SEEK )
				{
					projptr->flags &= ~QP_SORTMASK;
					projptr->flags |= QP_STRING;
				}
			}
#if 0
			else if ( strncmp( modstr, "sor", 3 ) == 0 ) {
				/* sort on this attribute */
				projptr->flags |= QP_SORT;
				if ( (projptr->flags & QP_SORTMASK) == 0 )
					projptr->flags |= dflt_sort;
				/* get optional priority */
				if ( numstr != NULL )
					projptr->priority = atoi( numstr );
			}
#endif
			else if ( strncmp( modstr, "split", 5 ) == 0 ) {
				/* delimiters for sub-field sort */
				projptr->flags |= QP_SORT;
				if ( (projptr->flags & QP_SORTMASK) == 0 )
					projptr->flags |= dflt_sort;
				if ( modstr[5] ) {
					projptr->delim = &modstr[5];
					/* remove back slashes in delimstr */
					cnvtbslsh( projptr->delim,
						projptr->delim );
				}
			}
			else
				good_modifier = FALSE;
			break;
		case 'v':
			if ( projptr->attr != ATTR_ALL )
			{
				/* application controls whether or not "value=" is to be recognized */
				if ( ( uqpnvalue == QP_NEWVALUE ) && ( strncmp( modstr, "value=", 6 ) == 0 ) )
				{
					if ( nextstr ) {
						*nextstr = ':';
						nextstr = NULL;
					}
					if ( ( ( projptr->attorval ) == NULL ) ||
					     ( projptr->flags & (QP_NEWVALUE|QP_NOVERBOSE) == QP_NEWVALUE ) )
					{
						cnvtbslsh( &modstr[6], &modstr[6] );
						if ( strchr( modstr, '\n' ) ) {
							if ( msgs ) {
								prmsg( MSG_ERROR, "value cannot have embedded new line" );
							}
							good_modifier = FALSE;
						} else {
							projptr->flags |= QP_NEWVALUE;
							projptr->attorval = &modstr[6];
						}
					} else {
						if ( msgs ) {
							prmsg( MSG_ERROR, "value and verbose are mutually exclusive modifiers" );
						}
						good_modifier = FALSE;
					}
				}
				else if ( strncmp( modstr, "verbose=", 8 ) == 0 )
				{
					if ( nextstr ) {
						*nextstr = ':';
						nextstr = NULL;
					}
					if ( ( ( projptr->attorval ) == NULL ) ||
					     ( projptr->flags & (QP_NEWVALUE|QP_NOVERBOSE) == QP_NOVERBOSE ) )
					{
						if ( strchr( modstr, '\n' ) ) {
							if ( msgs ) {
								prmsg( MSG_ERROR, "verbose cannot have embedded new line" );
							}
							good_modifier = FALSE;
						} else {
							projptr->flags |= QP_NOVERBOSE;
							projptr->attorval = &modstr[8];
						}
					} else {
						if ( msgs ) {
							prmsg( MSG_ERROR, "value and verbose are mutually exclusive modifiers" );
						}
						good_modifier = FALSE;
					}
				}
				else {
					good_modifier = FALSE;
				}
			} else {
				good_modifier = FALSE;
			}
			break;
		case 'w':
			if ( strncmp( modstr, "width", 5 ) == 0 ) {
				/* termination width for attr on output */
				char *digitptr = modstr + 5;
				while ( isdigit(*digitptr) )
					++digitptr;
				if ( *digitptr == '\0' && isdigit(modstr[5]) ) {
					int width = atoi( &modstr[5] );
					if ( width >= 0 && width < DBBLKSIZE) {
						projptr->attrtype = QP_FIXEDWIDTH;
						projptr->attrwidth = (unsigned short)width;
						projptr->terminate = '\0';
					}
				}
			}
			else
				good_modifier = FALSE;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			/* print width and possible justification */
			projptr->prwidth = 0;
			while( isdigit( *modstr ) ) {
				projptr->prwidth = projptr->prwidth * 10 +
							(*modstr - '0');
				modstr++;
			}
			switch( *modstr ) {	/* justification? */
			case 'c':
			case 'C':
			case 'l':
			case 'L':
			case 'r':
			case 'R':
				projptr->justify = *modstr;
				break;
			}
			break;
		default:
			good_modifier = FALSE;
			break;
		} /* end switch( *modstr ) */

		if ( ! good_modifier && msgs ) {
			prmsg( MSG_ERROR, "unrecognized attribute modifier '%s'",
				modstr );
			rc = FALSE;
		}

		if ( nextstr != NULL ) {
			*nextstr = ':';	/* put colon back */
			modstr = nextstr + 1;
		}
		else
			break;
	}

	return( rc );
}

lookupprojattr( fullname, nodelist, nodecnt, projptr, friendly )
char *fullname;
struct qnode *nodelist;
int nodecnt;
struct qprojection *projptr;
int friendly;
{
	register char *ptr;
	char *colonptr;
	struct uattribute *attrptr;
	char attrbuf[MAXPATHLEN + ANAMELEN + 1 + 4];	/* allow for "/./" or "././" prefix */

	projptr->attr = -1;

	/*
	 * Check for any modifiers on the attribute.
	 */
	colonptr = strchr( fullname, ':' );
	if ( colonptr )
	{
		*colonptr = '\0';

		/*
		 * Check for "all:nodisplay=" which is/can be used
		 * to indicate the special ATTR_ALL attribute even
		 * when the relation(s) contains a normal attribute
		 * with the name "all".
		 */
		if ( strncmp( colonptr+1, "nodisplay=", 10 ) == 0 ) {
			ptr = strchr( fullname, '.' );
			if ( ptr == NULL ) {
				ptr = fullname;
			} else {
				++ptr;
			}
			if ( strcmp( ptr, "all" ) == 0 ) {
				strcpy( attrbuf, fullname );
				if ( lookupnode( attrbuf, nodelist, nodecnt, &projptr->rel ) != NULL ) {
					projptr->attr = ATTR_ALL;
				}
			}
		}
	}

	if ( ( projptr->attr == -1 ) &&
	     ( ! _lookupattr( fullname, nodelist, nodecnt, &projptr->rel, &projptr->attr, TRUE ) ) ) {
		if ( colonptr )
			*colonptr = ':';
		return( FALSE );
	}

	projptr->flags = 0;
	projptr->priority = 0xffff;
	projptr->sortattr = 0;
	projptr->attorval = NULL;
	projptr->delim = NULL;
	projptr->subcnt = 0;
	switch( projptr->attr ) {
	case ATTR_RECNUM:
		projptr->prwidth = 6;
		projptr->prname = friendly ? "Record Number" : "rec#";
		projptr->justify = 'r';
		projptr->terminate = ':';
		projptr->attrwidth = 0;
		projptr->attrtype = QP_TERMCHAR;
		break;
	case ATTR_SEEK:
		projptr->prwidth = 6;
		projptr->prname = friendly ? "Seek Value" : "seek#";
		projptr->justify = 'r';
		projptr->terminate = ':';
		projptr->attrwidth = 0;
		projptr->attrtype = QP_TERMCHAR;
		break;
	case ATTR_ALL:
		projptr->prwidth = 0;
		projptr->prname = NULL;
		projptr->justify = '\0';
		projptr->terminate = '\0';
		projptr->attrwidth = 0;
		projptr->attrtype = -1;
		break;
	default:	/* normal attribute */
		attrptr = &projptr->rel->rel->attrs[ projptr->attr ];

		projptr->prwidth = attrptr->prwidth;
		projptr->justify = attrptr->justify;
		projptr->prname = friendly && attrptr->friendly ?
					attrptr->friendly :
					attrptr->aname;
		if ( ( attrptr->attrtype == UAT_TERMCHAR ) ||
		     ( attrptr->terminate >= DBBLKSIZE ) ) {
			projptr->attrtype = QP_TERMCHAR;
			projptr->terminate =
				(projptr->attr == projptr->rel->rel->attrcnt - 1) ?
				'\t' :
				(char)attrptr->terminate;
			projptr->attrwidth = 0;
		} else {
			projptr->attrtype = QP_FIXEDWIDTH;
			projptr->attrwidth = attrptr->terminate;
			projptr->terminate = '\0';
		}
		break;
	}

	if ( colonptr ) {
		*colonptr = ':';
		if ( ! attr_modifier( projptr, colonptr, TRUE ) )
			return( FALSE );
	}

	return( TRUE );
}
