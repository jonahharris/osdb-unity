/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

#define	QDBG_EXPR	0x0001
#define QDBG_HASH	0x0002
#define QDBG_JOIN	0x0004
#define QDBG_SELECT	0x0008
#define QDBG_IO		0x0010

extern int _qdebug;

extern int set_qdebug( /* char *debugstr */);
