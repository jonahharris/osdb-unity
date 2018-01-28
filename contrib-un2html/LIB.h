/*
 *
 * LIB GP header file
 *
 * LastEditDate="Fri Mar  4 23:45:00 1994"
 * Written by: R. de Heij
 */

/* Only load LIB.h if not already loaded */ 
#ifndef _LIB

#define _LIB

/*
 * The following macroes are defined for system performance reasons
 * The macroes allow inline coding. This reduces function call overhead
 */

#define FPUTC(chr, out)	\
    if (putc (chr, out) == EOF)\
	Quit ("Cannot write output", NULL)

#define FPUTS(string, out)	\
    if (fputs (string, out) == EOF)\
	Quit ("Cannot write output", NULL)

/* Definitions for Get_element() function in Getelmnt.c */

#define DOMLEN	1000
#define ELMLEN	100

/* Definitions for Met_graph in Met_graph.c */

#define METMAXLINES	8
#define METMAXPOINTS	1000
#define METDESCRLEN	30
#define METMAXHDR	10

typedef struct metxy {
	int x;			       /* date in Ddate format */
	int y;			       /* Yvalue */
} METXY;

extern char Met_lines[METMAXLINES][METDESCRLEN];  /* Description of the lines */
extern int Met_nrlines;			       /* Number of lines wanted */
extern METXY Met_x_y[METMAXLINES][METMAXPOINTS];  /* X_Y Points */
extern int Met_nrpoints[METMAXLINES];	       /* nr of points per line */

/* Definitions for Pln_graph in Pln_graph.c */

#define PLNLBLLEN	20
#define PLNDESCRLEN	(PLNLBLLEN + PLNLBLLEN)
#define PLNMAXENTRIES	50
#define PLNMAXMILESTONES	10
#define PLNMAXGRAPH	20
#define PLNMAXHDR	10

/* types of graphs */
#define PLNNORMALGRAPH	1
#define PLNHISTGRAPH	2
#define PLNALLGRAPH	3
#define PLNGRAPHTYPES	"1, 2, 3"

/* Pln entries */
typedef struct plnentry {
	char description[PLNDESCRLEN];
	int nrgraph;
	struct grf {
		char lbltop[PLNLBLLEN];
		char lblbot[PLNLBLLEN];
		char lblright[PLNLBLLEN];
		char lblleft[PLNLBLLEN];
		int s_planned;
		int e_planned;
		int s_rescheduled;
		int e_rescheduled;
		int s_actual;
		int e_actual;
	} graph[PLNMAXGRAPH];
} PLNENTRY;

/* Milestones */
typedef struct plnmilestone {
	char description[PLNDESCRLEN];
	int date;
} PLNMILESTONE;

extern PLNENTRY Pln_grph[PLNMAXENTRIES];	       /* Graph info */
extern int Pln_nrentries;			       /* Nr entries in graph */
extern PLNMILESTONE Pln_milestone[PLNMAXMILESTONES]; /* Milestone info */
extern int Pln_nrmilestones;		       /* Nr entries in milestone info */

#endif
