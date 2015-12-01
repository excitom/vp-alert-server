#define LANGUAGE   "us_english"
#define SQLBUFLEN  255
extern  void       error();

/* Forward declarations of the error handler and message handler routines. 
*/
int CS_PUBLIC err_handler PROTOTYPE((DBPROCESS *dbproc, 
	int severity, 
	int dberr, 
	int oserr,
	char *dberrstr,
	char *oserrstr
	));

int CS_PUBLIC msg_handler PROTOTYPE((
	DBPROCESS       *dbproc,
	DBINT           msgno,
	int             msgstate,
	int             severity,
	char            *msgtext,
	char            *srvname,
	char            *procname,
	int     	line
	));
