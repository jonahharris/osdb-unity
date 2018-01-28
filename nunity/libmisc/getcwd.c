/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/dir.h>

extern char *malloc();

#define MAXPATH	512

char *
getcwd( path, maxlen )
char *path;
int maxlen;
{
	struct stat buf1, buf2, tmp;
	struct direct dir;
	char *dotdot = "..";

	register struct stat *curdir, *parent, *tmpptr;
	register int len, tmpcnt;
	short dofree;
	FILE	*file;

	curdir = &buf1;
	parent = &buf2;
	dofree = (path == NULL);
	if ( dofree ) {
		if ( maxlen <= 0 )
			maxlen = MAXPATH + 4;	/* allow for "/./" or "././" prefix */
		path = malloc( (unsigned)( maxlen + 1 ) );
		if ( path == NULL )
			return( NULL );
	}

	*path = '\0';
	if ( stat( ".", curdir ) < 0 ) {
		if ( dofree )
			free( path );
		return( NULL );
	}

	len = 0;
	while( len < maxlen ) {
		if ( (file = fopen( dotdot, "r" )) == NULL ) {
			chdir( &path[1] );
			if ( dofree )
				free( path );
			else
				*path = '\0';
			return( NULL );
		}
		if ( fstat( fileno( file ), parent ) < 0 ) {
			fclose( file );
			chdir( &path[1] );
			if ( dofree )
				free( path );
			else
				*path = '\0';
			return( NULL );
		}
		if ( curdir->st_dev == parent->st_dev &&
				curdir->st_ino == parent->st_ino ) {
			fclose( file );
			if ( len == 0 ) {
				path[0] = '/';
				path[1] = '\0';
			}
				chdir( &path[1] );
			return( path );
		}
		chdir( dotdot );
		fseek( file, (long)(2 * sizeof( dir )), 0 ); /* skip . & .. */
		do {
			if ( fread( (char *)&dir, sizeof(dir), 1, file ) < 1 ) {
				fclose( file );
				chdir( &path[1] );
				if ( dofree )
					free( path );
				else
					*path = '\0';
				return( NULL );
			}
			stat( dir.d_name, &tmp );
		} while( curdir->st_ino != tmp.st_ino ||
			curdir->st_dev != tmp.st_dev );

		fclose( file );
		if ( (tmpcnt = cat( path, len, dir.d_name, maxlen)) == 0 ) {
			chdir( &path[1] );
			if ( dofree )
				free( path );
			else
				*path = '\0';
			return( NULL );
		}

		len += tmpcnt;

		tmpptr = curdir;		/* transfer stat info */
		curdir = parent;
		parent = tmpptr;
	}

	chdir( &path[1] );
	if ( dofree )
		free( path );
	else
		*path = '\0';
	return( NULL );
}

static int
cat( path, len, dirname, max )
register char *path;
register int len;
register char *dirname;
register int max;
{
	register int i, j;

	i = 0;
	while ( i < 14 && dirname[i] != '\0' )
		i++;
	++i;		/* allow for '/' */
	if ( i + len + 1 >= max )
		return( 0 );

	for( j = len + 1 ; j >= 0; j-- )
		path[j + i] = path[j];
	
	*path++ = '/';
	for( j = i - 2; j >= 0; j-- )
		*path++ = *dirname++;

	return( i );
}
