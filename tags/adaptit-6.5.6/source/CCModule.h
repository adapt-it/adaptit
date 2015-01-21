/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CCModule.h
/// \author			Bill Martin
/// \date_created	15 March 2008
/// \rcs_id $Id$
/// \copyright		See below for original copyright notice for original cc source code.
///                 Modified and adapted 2008 by permission for inclusion in adaptit 
///                 by Bruce Waters, Bill Martin, SIL International.
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CCCModule class. 
/// The CCCModule class encapsulates the traditional SIL Consistent Changes functionality
/// in a single C++ class. It only captures the CC functions which load a .cct table and
/// process consistent changes defined by that table on a buffer.
/// \derivation		The CCCModule class is not a derived class
/////////////////////////////////////////////////////////////////////////////
// * PROPRIETARY RIGHTS NOTICE:  All rights reserved.  This material contains
// *		the valuable properties of Jungle Aviation and Radio Service, Inc.
// *		of Waxhaw, North Carolina, United States of America (JAARS)
// *		embodying substantial creative efforts and confidential information,
// *		ideas and expressions, no part of which may be reproduced or transmitted
// *		in any form or by any means, electronic, mechanical, or otherwise,
// *		including photocopying and recording or in connection with any
// *		information storage or retrieval system without the permission in
// *		writing from JAARS.
// *
// * COPYRIGHT NOTICE:  Copyright <C> 1980, 1983, 1987, 1988, 1991, 1993,
// *    1994, 1995, 1996,1997, 1998
// *		an unpublished work by the Summer Institute of Linguistics, Inc.
/////////////////////////////////////////////////////////////////////////////
// Notice/Disclaimer:
// My main goal in creating this CCCModule was to encapsulate the Consistent
// Changes functionality for use in the wxWidgets based cross-platform 
// versions of Adapt It so that the same code base could build versions of
// Adapt It for Windows, Linux and the Macintosh, without resort to creating 
// separate stand alone .dll or .so dynamic libraries. This CCCModule can 
// be plugged in to almost any other wxWidgets based application without need 
// for further changes.
// 
// I have tried to preserve all the original cc algorithms as is - at least
// I have tried to avoid accidentally messing with them. The most significant 
// changes I've made relate to file i/o. A lot could still be done
// to bring the code up to more modern standards, avoid use of fixed buffers,
// remove deprecated library functions, etc. The reader will also note that 
// I have not attempted to revise any of the original authors' comments that 
// document the various functions taken from the various Consistent Changes 
// source files. Hence, many of them refer to parameters or variables that 
// are no longer part of the functions. Most variables that were declared 
// in global scope as static variables in various source files are no longer 
// declared as static. To make the code easier to follow, I've also eliminated
// most of the numerous conditional defines. -whm.
/////////////////////////////////////////////////////////////////////////////

#ifndef CCModule_h
#define CCModule_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CCModule.h"
#endif

// whm: we are not using a dll for CCCModule, but we want to process cc within buffers rather than repeatedly loading external
// files, processing them with cc and rewriting the results to external files again, so the functionality we want is
// similar to what is achieved in the _WINDLL defined blocks. 
// In CCCModule we've gotten rid of the _WINDLL define because the code it defined conditionally is
// always part of the CCCModule.

typedef unsigned long	UCS4;
typedef unsigned char	UTF8;
#define MAXUTF8BYTES 6
#define UTF8SIZE 6

// The original cc sources had #include "windows.h" and #include "setjmp.h" at this point 
// for when _WINDLL was defined

// the VERSIONMAJOR and VERSIONMINOR below combine to form the version of
// the release.  Note that a '.' is automatically stuck in between them.
// They should both be numeric values for a release.  (It is OK for a beta
// to have some beta level info after the number part of VERSIONMINOR).
#define VERSIONMAJOR   "8"
#define VERSIONMINOR   "1.6-UTF8"

#define BUFFERSIZE 2048
#define UNGETBUFFERSIZE 100

/* Local equates--for type of search in the symbol table */
#define NAME_SEARCH		1
#define NUMBER_SEARCH	0

typedef struct
{
    char Buffer[BUFFERSIZE+UNGETBUFFERSIZE];
    unsigned iBufferFront;
    unsigned iBufferEnd;
    bool fEndOfFile;
	// whm modified FILE* and HFILE declarations below to use a pointer to wxFile (wxFile* hfFile) for
	// both UNIX and non-UNIX platforms. Note: this pointer is assigned to an instance of wxFile which
	// is created on the heap in wfopen(), and deleted in wfclose().
	wxFile* hfFile;
    bool fTextMode;
    bool fWrite;
    long FilePos;
    unsigned int fuMode;
} WFILE;


/* Indices into the above array */
#define NON_NUMBER		0
#define BIG_NUMBER		1
#define CCOVERFLOW		2 // whm changed from OVERFLOW to CCOVERFLOW because of conflict with VS8's math.h
#define DIVIDE_BY_ZERO	3

// define return codes from the CC DLL routines 
#define CC_SUCCESS                   0
#define CC_GOT_FULL_BUFFER           0
#define CC_GOT_END_OF_DATA           1
#define CC_SUCCESS_BINARY            1
#define CC_CALL_AGAIN_FOR_MORE_DATA  2
#define CC_SYNTAX_ERROR             -2

#define REFERENCED	1
#define DEFINED		2

#define STATUSUPDATEINCREMENT 1

// the original cc sources defined CARRIAGERETURN for _Mac as '\r' instead of the octal '\015'
#define CARRIAGERETURN	'\015'

#define TAB				    '\011'
#define CTRLZ				'\032'
#define CTRLS				'\023'
#define CTRLC				'\003'
#define FORMFEED			'\014'
#define BACKSPACE			'\010'
#define ESCAPE				'\033'
#define HT					'\011'
#define FIXEDSPACE			'#'
#define VERTICALBAR			'|'

enum ccMsgFormatType
{
	MSG_S_S,		// formatted string has two %s parameters (see commented out define above)
	MSG_S_C_S,		// formatted string has two %s and one %c parameters (see commented out define above)
	MSG_S,			// formatted string has one %s parameter (see commented out define above)
	MSG_noparms,	// formatted string has no parameters (see commented out define above)
	MSG_LD			// formatted string has one %ld (long int) parameter (see commented out define above)
};

enum ccMsgType
{
	WARNING_MESSAGE,	// message is warning or informational
	ERROR_MESSAGE		// message means nasty error problem
};

/* Equates for referencing the symbol table array */

#define STORE_HEAD	0
#define SWITCH_HEAD	1
#define GROUP_HEAD	2
#define DEFINE_HEAD	3

/* Boolean constants */
#ifndef TRUE
#define TRUE			(1)
#endif
#ifndef FALSE
#define FALSE			(0)
#endif

/* Memory contraints */
#define MAX_ALLOC		65536*4 - 1

//ref MSDN Q32998
#define RESERVEDSPACE	4096		/* Reserved space for file buffers */

/* Exit flags */
#define GOODEXIT		0
#define BADEXIT			1

/* Equates */
#define WEDGE			'>'			/* separator of search from replacement */
#define HIGHBIT         0x8000		/* Used to test high order bit of elements */
/* in a search element */
#define LINEMAX			500			/* line is cut off if more than max */

#define STACKMAX		10			/* max level of do nesting */
#define STORESTACKMAX   10			/* max number of nested pushstore commands */

#define MAXCHANGES		16000		/* max number of changes allowed */
#define MINMATCH        300			/* the minimum number of bytes in the match buffer */

/* so we can show something in the debug input buffer display */
#define MAXMATCH		20500		/* match area len, BACKBUFMAX+maxsrch+500 */
/* the extra 500 are for the debug input buffer display */
#define BACKBUFMAX		20000		/* length of backup ring buffer */

#define MAXARG			1000		/* maximum allowable command argument */
#define MAXDBLARG       255			/* max allowable doublebyte command arg */
#define MAXGROUPS		MAXARG		/* number of groups allowed */
#define GROUPSATONCE	25			/* number of groups allowed on at once */
#define NUMSTORES		MAXARG+1	/* number of store areas plus 1 */
#define SRCHLEN_FACTOR	10			/* Length factor for srchlen() routine */
#define MAX_STACKED_PREC 10			/* Maximum number of prec commands on left of wedge */
#define BEFORE_CNT		35			/* # of characters from backup ring */
/*   buffer to be shown in debug displays */
#define AFTER_CNT		35			/* # of characters from input buffer */
/*   to be shown in debug displays */

#define NUMBER_FILES 300
#define PATH_LEN 250             /* allows for a path and file name */

#define PRIMARY_LEN 250
#define EXT_LEN 3

#define ARRAY_SIZE (PRIMARY_LEN+EXT_LEN+2)
#define ARRAY_SIZE_2 (ARRAY_SIZE-1)

#define AUX_BUFFER_LEN		4096     // if we need auxiliary output buffer start
// with this size and reallocate if needed
#define   INPUTBUFFERSIZE   1024        // this is a rather arbitrary number
#define   OUTPUTBUFFERSIZE  1024        // this is a rather arbitrary number

/* Change table commands
 *
 * Note: The dis-continuities in the sequence are due to old commands
 *			  which are no longer accepted, but which should not be used in the
 *			  future.  If a new command is needed, take the next-higher
 *			  negative number, to avoid incompatibilities.
 *
 *			Name		Decimal			  Octal			Hex
 */
#define FONTCMD			(-1)			/* 377			FF */
#define SPECPERIOD		(-2)			/* 376			FE */
#define SPECWEDGE		(-3)			/* 375			FD */
#define IFCMD			(-10)			/* 366			F6 */
#define IFNCMD			(-11)			/* 365			F5 */
#define ELSECMD			(-12)			/* 364			F4 */
#define ENDIFCMD		(-13)			/* 363			F3 */
#define SETCMD			(-14)			/* 362			F2 */
#define CLEARCMD		(-15)			/* 361			F1 */
#define STORCMD			(-16)			/* 360			F0 */
#define ENDSTORECMD		(-17)			/* 357			EF */
#define OUTCMD			(-18)			/* 356			EE */
#define DUPCMD			(-19)			/* 355			ED */
#define APPENDCMD		(-20)			/* 354			EC */
#define BACKCMD			(-21)			/* 353			EB */
#define FWDCMD			(-22)			/* 352			EA */
#define OMITCMD			(-23)			/* 351			E9 */
#define NEXTCMD			(-24)			/* 350			E8 */
#define IFEQCMD			(-25)			/* 347			E7 */
#define IFNEQCMD		(-26)			/* 346			E6 */
#define INCRCMD			(-27)			/* 345			E5 */
#define BEGINCMD		(-28)			/* 344			E4 */
#define ENDCMD			(-29)			/* 343			E3 */
#define REPEATCMD		(-30)			/* 342			E2 */
#define TOPBITCMD       (-31)	        /* 341          E1 */ /* no longer needed? */
#define DEFINECMD		(-32)			/* 340			E0 */
#define DOCMD			(-33)			/* 337			DF */
#define GROUPCMD		(-34)			/* 336			DE */
#define USECMD			(-35)			/* 335			DD */
#define WRITCMD			(-36)			/* 334			DC */
#define WRSTORECMD		(-37)			/* 333			DB */
#define READCMD			(-38)			/* 332			DA */
#define INCLCMD			(-39)			/* 331			D9 */
#define CASECMD			(-41)			/* 327			D7 */
#define ANYCMD			(-42)			/* 326			D6 */
#define PRECCMD			(-43)			/* 325			D5 */
#define FOLCMD			(-44)			/* 324			D4 */
#define WDCMD			(-45)			/* 323			D3 */
#define CONTCMD			(-46)			/* 322			D2 */
#define OUTSCMD			(-47)			/* 321			D1 */
#define IFGTCMD			(-49)			/* 317			CF */
#define ADDCMD			(-50)			/* 316			CE */
#define SUBCMD			(-51)			/* 315			CD */
#define MULCMD			(-52)			/* 314			CC */
#define DIVCMD			(-53)			/* 313			CB */
#define EXCLCMD			(-54)			/* 312			CA */
#define MODCMD			(-55)			/* 311			C9 */
#define BINCMD			(-56)			/* 310			C8 */
#define UNSORTCMD		(-57)			/* 307			C7 */
#define IFSUBSETCMD		(-58)			/* 306			C6 */
#define LENCMD			(-59)			/* 305			C5 */
#define PUSHSTORECMD    (-60)			/* 304          C4 */
#define POPSTORECMD     (-61)			/* 303          C3 */
#define VALCMD          (-62)			/* 302          C2 */
#define ENDFILECMD      (-63)			/* 301          C1 */
#define DECRCMD         (-64)			/* 300          C0 */
#define DOUBLECMD       (-65)			/* 277			BF */
#define IFNGTCMD        (-66)			/* 276			BE */
#define IFLTCMD         (-67)			/* 275			BD */
#define IFNLTCMD        (-68)			/* 274			BC */
#define UTF8CMD			(-69)			/* 273			BB */
#define BACKUCMD		(-70)			/* 272			BA */
#define FWDUCMD			(-71)			/* 271			B9 */
#define OMITUCMD		(-72)			/* 270			B8 */
#define ANYUCMD			(-73)			/* 269			B7 */
#define PRECUCMD		(-74)			/* 268			B6 */
#define FOLUCMD			(-75)			/* 267			B5 */
#define WDUCMD			(-76)			/* 266			B4 */

// values follow for signaling if predefined stores found for out command
#define CCCURRENTDATE  1               /* cccurrentdate predefined store */
#define CCCURRENTTIME  2               /* cccurrenttime predefined store */
#define CCVERSIONMAJOR 3               /* ccversionmajor predefined store */
#define CCVERSIONMINOR 4               /* ccversionminor predefined store */

// BEW 21Aug14 in cmstruct, void	(*cmfunc) (char, char, char); will not
// work as expected, because the functions have been made CCCModule member
// functions, and so cmfunc will not get a valid function pointer at compile time,
// and will just be left NULL. My workaround is to replace the function ptr in
// cmstruct with an enum which has 4 values, one for each of the 4 functions
// we need to support: storenoarg, storearg, storoparg, stordbarg
enum WhichFunc {
	nothing,
	storenoarg,
	storearg,
	storeoparg,
	storedbarg
}; // note, these enum names are just the old function names with e inserted after stor
struct cmstruct
{
    char	 *cmname;			/* Name of operation */
	//void	(*cmfunc) (char, char, char);		 /* Function to call to compile it */ deprecated 21Aug14
    enum	WhichFunc cmfunc; // to use this, add 'e' character in the relevant tables
	// so that stornoarg becomes storenoarg, storarg becomes storearg, etc, in
	// CCModule.cpp at lines 400++ approx
    char	cmcode;				/* Compiled code */
    char	symbolic;			/* Non-zero == command can take symbolic arguments */
    char	symbol_index;		/* If (symbolic)
    								*	 index into the symbol table array
    								*/
};

/* Types */

typedef signed short int SSINT; /* will work same in 16-bit or 32-bit mode */
typedef SSINT *tbltype;         /* Pointer into table[] */
typedef SSINT **cngtype;        /* Pointer into cngpointers[] */

//typedef char bool;				  /* booleans stored in byte */  /* whm commented out */
typedef char filenametype[80 + 1];		/* for getln routine */

/* Symbol table element for CC */
typedef struct symnode
{
    struct symnode *next;	/* Pointer to next symbol */
    char *name;					/* Pointer to name string */
    int number;					/* Number of store, switch, define, or group */
    char use;					/* Some combination of the equates REFERENCED */
    /* and DEFINED (declared below) */
} CC_SYMBOL;

typedef struct sym_tab
{
    CC_SYMBOL *list_head;	 /* Pointer to the list */
    int		  next_number;  /* Next (possibly) available number */
} SYM_TAB;


// the typedefs below are for error/warning message generation

typedef  struct msg_S_S_struct
{
    wxString string1;
    wxString string2;
} MSG_STRUCT_S_S;

typedef  struct msg_S_C_S_struct
{
    wxString string1;
    wxChar char1;
    wxString string2;
} MSG_STRUCT_S_C_S;


/*
 * NOTE: ANY GLOBAL VARIABLES THAT ARE CHANGED IN OR ADDED TO THE LIST OF
 *       GLOBAL VARIABLES BELOW MUST ALSO BE CHANGED THE SAME WAY IN THE
 *       STRUCTURE THAT FOLLOWS, UNLESS THEY ARE RELATED TO INPUT/OUTPUT
 *       FILES AND DO NOT AFFECT THE DLL.  THEY MUST ALSO BE REFLECTED IN
 *       THE SaveState() AND RestoreState() ROUTINES AS WELL.
 *
 */

/* debugging version variables */
#define MAXTABLELINES 32767

struct cc_global_vars //typedef struct cc_global_vars
{
    unsigned iCurrentLine;     /* current line in cc table */
    const void * hTableLineIndexes;  /* handle of array of indexes into the compiled cc table for the current line */
    SYM_TAB sym_table[4];      /* the array itself  */
    wxString tblarg; //char *tblarg;              /* pointer to table file name if cmdlni */
	WFILE *tablefile;			// table file for compiled table
    bool bEndofInputData;                /* on if end of file */
    long nextstatusupdate;
    char  nullname[1];         /* "" */
    char line[LINEMAX+1];      /* input line (changes and input) */
    /* vars specific to compile */
    char *parsepntr,
    *parse2pntr;               /* pointers into line */
    bool fontsection;          /* old-style font section present in table */
    bool notable;              /* on if no table */
    bool was_math;             /*   on if last command parsed was a math
    										  * operator (add, sub, mul, div, incr)
    										  */
    bool was_string;           /*   on if last command parsed was a
    										  * quoted string
    										  */
    tbltype executetableptr;   /* ???Pointer into replacement side of a change when
    								* an execute terminates because of a full output buffer */
    tbltype table;             /* table of changes (Dynamically allocated) */
    tbltype tablelimit;        /* tablemax variable for MS and CC */
    tbltype tloadpointer;      /* pointer for storing into table */
    tbltype maintablend;       /* end of main table before fonts */
    SSINT *storelimit;         /* Limit of store area */
    char switches[MAXARG+1];           /* switches */
    tbltype storebegin[NUMSTORES+1];   /* beg ptrs to store in table[] */
    tbltype storend[NUMSTORES+1];      /* end ptrs +1 to store in table[] */
    char storeact[NUMSTORES+1];        /* TRUE -> store active in matching */
    char storepre[NUMSTORES+1];        /* predefined store found for out command
                                          if element if this is nonzero    */
    int curstor;                  /* current storing area */
    int iStoreStackIndex;         /* store() stack pointer */
    int storeoverflow;            /* prevents repeating message */
    int doublebyte1st;            /* first boundary value for doublebyte mode     */
    int doublebyte2nd;            /* optional second boundary for doublebyte mode */
    cngtype groupbeg[MAXGROUPS+1];   /* beginning of group */
    cngtype groupxeq[MAXGROUPS+1];   /* beginning of leftexec's of group */
    cngtype groupend[MAXGROUPS+1];   /* beginnings and ends of groups */
    int curgroups[GROUPSATONCE+1];   /* current groups in use */
    int cgroup;                      /* group from which currently executing
                                      * change came
                                      */
    int numgroupson;              /* number of groups currently on */
    char letterset[256];          /* current set of "active" match chars */
    int setcurrent;               /* TRUE -> letterset current */
    tbltype defarray[MAXARG+1];   /* defines for do */
    SSINT *stack[STACKMAX+1];     /* stack for do command */
    int stacklevel;               /* level of do nesting */
    SSINT *backbuf;               /* ring buffer for backup */
    SSINT *backinptr;             /* pointers into backbuf */
    SSINT *backoutptr;
    SSINT *dupptr;                /* Pointer into match buffer for 'dup' command */
    tbltype *cngpointers;         /* change ptrs into table[] */
    tbltype *cngpend;             /* last cngpointer +1 */
    SSINT *match;                 /* match area for searching */
    SSINT *matchpntr;             /* pointer into match area */
    SSINT *matchpntrend;
    int maxsrch;                  /* length of longest search string */
    bool errors;                  /* set if errors in table */
    bool bFileErr;               /* set if errors opening files */
    bool tblfull;                /* set if table too large */
    bool eof_written;            /* set when CC outputs CTRLZ */
    bool debug, mandisplay, mydebug, single_step;  /* debug mode switches */
    bool caseless, uppercase;    /* caseless mode switches */
    bool unsorted;               /* TRUE if table is processed unsorted */
    bool binary_mode;            /* TRUE if table is processed in binary mode */
    bool doublebyte_mode;        /* TRUE if using double byte input mode */
    bool doublebyte_recursion;   /* TRUE only for recursive call */
    bool utf8encoding;		     /* TRUE if input/output is UTF-8 encoded */
    bool quiet_flag;             /* quiet mode flag */
    bool noiseless;              /* noiseless mode flag (no beeps or messages) 7.4.22 BJY */
    int hWndMessages;
    /* global variables that were originally specific to one source file  */
    /* cciofn.c formerly static variable */
    int StoreStack[STORESTACKMAX];  /* Used by PushStore and PopStore */
    /* ccexec.c formerly static variables */
    int mchlen[2];       /* xeq, letter match lengths */
    int matchlength;     /* length of last match */
    tbltype tblxeq[2];   /* xeq, letter table pointer */
    tbltype tblptr;      /* working pointer into table */
    SSINT *mchptr;       /* working pointer into match */
    SSINT firstletter, cngletter;   /* letters during matching */
    int cnglen;          /* Length of change actually being executed */
    /*  used for debug display */
    int endoffile_in_mch[2]; /* used to flag an endoffile on the left side of a possible match
    											 added 7/31/95 DAR */
    int endoffile_in_match; /* used to flag an endoffile on the left side of a match
                                       added 7/31/95 DAR */
    SSINT *store_area;  /* Store area (dynamically allocated) */ //7.4.15 BDW store_area now has file scope rather than scope within ccstart()
    /* cccomp.c formerly static variable */
    char keyword[20+1];         /* keyword for identification          */
    bool begin_found;           /* TRUE == we have a begin statement   */
    /* cccompfn.c formerly static variable */
    char *precparse;            /* used by chk_prec_cmd and compilecc  */
    
	/* ccstate.c formerly static variable */
    int nInBuf;                 // size of the cc input buffer passed to DLL
    int nInSoFar;               // number of bytes passed in so far this call
    bool bPassedAllData;        // true if we have already been passed all of
    // our input data from user callback routine
    char *lpOutBuf;             // points to cc output buffer for DLL
    char *lpOutBufStart;        // start of CC output buffer when CC is back end
    int nMaxOutBuf;             // maximum size of CC output buffer for DLL
    int nUsedOutBuf;            // used size of CC output buffer for DLL
    bool bMoreNextTime;         // TRUE if more data but filled output buffer
    bool bNeedMoreInput;
    bool bOutputBufferFull;
    bool bFirstDLLCall;         // TRUE if DLL is being called the first time
    bool bBeginExecuted;        // TRUE if begin statement was executed or it does not exist
    const void * hInputBuffer;        // handle for CC DLL input buffer area
    const void * hOutputBuffer;       // handle for CC DLL output buffer area
    char *lpInBuf;              // points to cc input buffer for DLL
    char *lpInBufStart;         // points to start of cc input buffer for DLL
    bool bSavedDblChar;         // TRUE if saved second half of doublebyte
								// char that needs to be outputted to the user
    char dblSavedChar;          // saved second half of doublebyte character
    bool bSavedInChar;          // TRUE if saved possible first half double-
								// byte char from last byte of input buffer
    char inSavedChar;           // saved possible first half of doublebyte
    char *lpAuxBufStart;        // start of special auxiliary output buffer
    char *lpAuxOutBuf;          // next write position in auxiliary buffer
    char *lpAuxNextToOutput;    // next aux buffer position to be outputted
    bool bAuxBufUsed;           // true if we have allocated aux buf to use
    bool bProcessingDone;       // true if DLL return completion rc to user
    bool bPassNoData;           // true if DLL needs no data next time called
    int nCharsInAuxBuf;         // length of used (filled) part of aux buffer
    int iAuxBuf;
    int nAuxLength;             // length of special auxiliary output buffer
    const void * hAuxBuf;             // handle for special auxiliary output buffer
    const void * hStoreArea;          // handle for our store area
    long lDLLUserInputCBData;   // user data passed to input callback function
    long lDLLUserOutputCBData;  // user data passed to output callback function
    long lDLLUserErrorCBData;   // user data passed to error callback function
    const void * hCCParent;           // handle for calling program
    wxString cctpath; //char cctpath[PATH_LEN+1];   // cct file path (includes null at end)
    unsigned short wDS;                   // ???
}; // GLOBAL_VARS_STRUCT;

/// The CCCModule class encapsulates the SIL Consistent Changes functionality
/// within Adapt It.
/// \derivation		CCCModule is not a derived class.
class CCCModule
{
public:
	CCCModule(void); // constructor
	virtual ~CCCModule(void); // destructor
	// other methods
	void CallCCMainPart();
	int CCReinitializeTable();
	int CCLoadTable(wxString lpszCCTableFile);
	int CCLoadTableCore(wxString lpszCCTableFile);
	int CCUnloadTable();
	int CCProcessBuffer (char *lpInputBuffer, int nInBufLen,
                        char *lpOutputBuffer, int *npOutBufLen);
	int CCSetUTF8Encoding(bool lutf8encoding);
	void out_char_buf(char inchar);
	int CleanUp();
	void Process_msg(short nMsgIndex, wxString errStr, long unsigned lParam);
	void SaveState();
	void RestoreState();
	void *tblalloc(unsigned count, unsigned size);
	void tblcreate();
	void compilecc();
	void fillmatch();
	void execcc();
	void startcc();
	void freeMem();
	void tblfree();
	void storefree();
	void storechar(SSINT ch);
	void storeelement(int search);
	void groupinclude(register int group);
	void groupexclude(register int group);
	int srchlen(tbltype tpl);
	bool wedgeinline();
	void flushstringbuffer(int search);
	void storestring(int search, char * string, int sLen);
	void inputaline();
	void output(register SSINT ch);
	void storch(register int store, SSINT ch);
	void tblskip(SSINT **tblpnt);
	void PushStore();
	void PopStore();
	void resolve_predefined(int index);
	void BackCommand(SSINT **cppTable, bool utf8);
	int valnum(register SSINT * operand, char *opbuf, int buflen, long int *value, char first);
	int storematch(SSINT **tblpnt);
	void LengthStore(SSINT **cppTable);
	int IfSubset(SSINT **tblpnt);
	void FwdOmitCommand(SSINT **cppTable, bool *pbOmitDone, bool fwd, bool utf8);
	SSINT inputchar();
    int UCS4toUTF8(UCS4 ch, UTF8 * pszUTF8);
    int UTF8AdditionalBytes(UTF8 InitialCharacter);
	SSINT * utf8characterfrombackbuffer(SSINT** buffer);
	void incrstore(int j);
	void decrstore(int j);
	void ccmath(SSINT oprtor, SSINT **tblpnt); // whm: changed parameter from operator (reserved in C++) to oprtor
	void math_error(char *op_1, char *op_2, char operation, int err_type, int storenum);
	long int long_abs(long value);
	void bailout(int WXUNUSED(exit_code), int WXUNUSED(ctrlc_flag));
	void completterset();
	int gmatch(int group);
	int cmatch(register cngtype cp, int ofs);
	int leftexec(int ofs);
	int anyutf8check(SSINT * mcptr);
	int anycheck(SSINT mc);
	int contcheck();
	void out_char(SSINT ch);

	void parse(char **pntr, char **pntr2, int errormessage);
	bool chk_prec_cmd();
	void discard_symbol_table();
	void check_symbol_table();
	int msg_putc(int ch);
	int msg_puts(char *s);
	int cctsetup();
	void doublebytestore(SSINT doublearg);

	void cctsort();
	bool goes_before(tbltype x, tbltype y);
	void fontmsg();
	WFILE * wfopen(wxString pszCCTFile, wxString mode);
	int wfclose(WFILE * stream);
	bool wfeof(WFILE * stream);
	int wgetc(WFILE * stream);
	int wungetc(int ch, WFILE * stream);
	int wfflush(WFILE * stream);
	SSINT *ssbytcpy(SSINT *d, SSINT *s, int n);
	int odd(register int i);
	int a_to_hex(register char ch);
	char *bytset(char *d, char c, int n);
	SSINT *ssbytset(SSINT *d, SSINT c, int n);
	SSINT *execute(int mlen, SSINT * tpx, int beginflag);
	unsigned int max_heap();
	void upcase_str(wxString &s);
	void refillinputbuffer();
	char *sym_name(int number, int type);
	void ccin();
	void writechar();
	int symbol_number(int sym_type, char sym_use);
	SSINT parsedouble(int *myflag);
	void err(char * message);
	int search_table(int table_index, char sym_usage, char *sym_name, int sym_num, int type_of_search);
	int hex_decode(char * pstr);
	int ucs4_decode(char * pstr);
	void decimal_decode(int * number);
	void octal_decode(register int *number);
	void buildkeyword();
	void stornoarg(char comand, char dummy1, char dummy2);
	void storarg(char comand, char sym_args, char table_head);
	void storoparg(char comand, char dummy1, char dummy2);
	void stordbarg(char comand, char dummy1, char dummy2);
	char *cmdname(char cmd, int search);

protected:

private:
	cc_global_vars ccGlobals; // instance of cc_global_vars struct used by SaveState() and RestoreState() 

/* Consistent Change global variables */
// whm: iCurrentLine and hTableLineIndexes have counterparts in ccGlobals (used by SaveState and
// RestoreState), but TableLineIndexes does not. 
// 
unsigned iCurrentLine;     /* current line in cc table */
const void * hTableLineIndexes;    /* ???handle of array of indexes into the compiled cc table for the current line */
// TableLineIndexes below was only declared when _WINDLL in the original cc sources was defined.
// 
// Note: In compilecc() there is a test to see if TableLineIndexes[i] == -1 (a negative number). 
// This appears to be comparing the address of an array with a negative number. GCC gives a warning 
// that says, "warning: comparison between signed and unsigned integer expressions."
unsigned * TableLineIndexes; /* array of indexes into the compiled cc table for the current line */

MSG_STRUCT_S_S Msg_s_s;
MSG_STRUCT_S_C_S Msg_s_c_s;

char errorLine[3*LINEMAX];    // place to store syntax error message

SYM_TAB sym_table[4];      /* the array itself  */
wxString filenm; //filenametype filenm;       /* ???input name from keyboard */
int namelen;               /* ???length of filename */
bool cmdlni;               /* ???True if input on command line */
wxString tblarg; //char *tblarg;              /* pointer to table file name if cmdlni */
char *outarg;              /* ???pointer to output file name if cmdlni */
char *inarg;               /* ???ponter to input file name */
char *writearg;            /* ???pointer to write output file if cmdlni */
int inparg;                /* ???number in argv of current input file */
int argcnt;                /* ???holds value of argc */
char **argvec;             /* ???holds value of argv */
WFILE *tablefile;			// table file for compiled table
FILE *msgfile;             /* ???all messages use this descriptor */
bool bEndofInputData;		/* on if end of file */
long nextstatusupdate;
long infilelength;         /* ???length of current input file. used for process
												 status */
long infileposition;       /* ???position in current input file */

char  nullname[1];		   /* "" */
long  outsize;             /* ???size of output disk file -1 otherwise  */

char line[LINEMAX+1];      /* input line (changes and input)         */
char strBuffer[LINEMAX+1]; /* ???buffer to be used for error message    */
char * pstrBuffer;         /* ???pointer to above buffer for messages */

/* vars specific to compile */
char *parsepntr,
*parse2pntr;               /* pointers into line */
bool fontsection;		   /* old-style font section present in table */
bool notable;			   /* on if no table */
bool was_math;			   /* on if last command parsed was a math
											  * operator (add, sub, mul, div, incr)
											  */
bool was_string;		   /* on if last command parsed was a
											  * quoted string
											  */
tbltype table;             /* table of changes (Dynamically allocated) */
tbltype tablelimit;        /* tablemax variable for MS and CC */
tbltype tloadpointer;	   /* pointer for storing into table */
tbltype maintablend;	   /* end of main table before fonts */
SSINT *storelimit;         /* Limit of store area */
char switches[MAXARG+1];            /* switches */
tbltype storebegin[NUMSTORES+1];	/* beg ptrs to store in table[] */
tbltype storend[NUMSTORES+1];		/* end ptrs +1 to store in table[] */
char storeact[NUMSTORES+1];			/* TRUE -> store active in matching */
char storepre[NUMSTORES+1];         /* predefined store found for out
                                             command if this is nonzero   */
int curstor;				  /* current storing area */
int iStoreStackIndex;         /* store() stack pointer */
int storeoverflow;            /* prevents repeating message */
int doublebyte1st;            /* first boundary value for doublebyte mode     */
int doublebyte2nd;            /* optional second boundary for doublebyte mode */
cngtype groupbeg[MAXGROUPS+1];	 /* beginning of group */
cngtype groupxeq[MAXGROUPS+1];	 /* beginning of leftexec's of group */
cngtype groupend[MAXGROUPS+1];	 /* beginnings and ends of groups */
int curgroups[GROUPSATONCE+1];	 /* current groups in use */
int cgroup;						 /* group from which currently executing
								  * change came
								  */
int numgroupson;			  /* number of groups currently on */
char letterset[256];          /* current set of "active" match chars */
int setcurrent;				  /* TRUE -> letterset current */
tbltype defarray[MAXARG+1];	  /* defines for do */
SSINT *stack[STACKMAX+1];     /* stack for do command */
int stacklevel;				  /* level of do nesting */
SSINT *backbuf;               /* ring buffer for backup */
SSINT *backinptr;             /* pointers into backbuf */
SSINT *backoutptr;
SSINT *dupptr;                /* Pointer into match buffer for 'dup' command */
tbltype *cngpointers;         /* change ptrs into table[] */
tbltype *cngpend;	          /* last cngpointer +1 */
tbltype executetableptr;      /* ???Pointer into replacement side of a change when
									* an execute terminates because of a full output buffer */
SSINT *match;                 /* match area for searching */
SSINT *matchpntr;             /* pointer into match area */
SSINT *matchpntrend;
int maxsrch;				  /* length of longest search string */
bool errors;				  /* set if errors in table */
bool bFileErr;				  /* set if errors opening files */
bool tblfull;				  /* set if table too large */
bool eof_written;			  /* set when CC outputs CTRLZ */
bool debug, mandisplay, mydebug, single_step;	/* debug mode switches */
bool caseless, uppercase;	  /* caseless mode switches */
bool unsorted;			      /* TRUE if table is processed unsorted */
bool binary_mode;		      /* TRUE if table is processed in binary mode */
bool doublebyte_mode;         /* TRUE if using double byte input mode */
bool doublebyte_recursion;    /* TRUE only for recursive call */
bool utf8encoding;	          /* TRUE if input/output is UTF-8 encoded */
bool quiet_flag;			  /* quiet mode flag */
bool noiseless;			      /* noiseless mode flag (no beeps or messages) 7.4.22 BJY */

int hWndMessages;
/* global variables that were originally specific to one source file  */
/* cciofn.c formerly static variable */
int StoreStack[STORESTACKMAX];  /* Used by PushStore and PopStore */
/* ccexec.c formerly static variables */
int mchlen[2];       /* xeq, letter match lengths */
int matchlength;     /* length of last match */
tbltype tblxeq[2];   /* xeq, letter table pointer */
tbltype tblptr;      /* working pointer into table */
/* Note:  The above variables are declared separately because */
/*          DECUS C does not allow regular typedef's  */
SSINT *mchptr;       /* working pointer into match */
SSINT firstletter, cngletter;   /* letters during matching */
int cnglen;          /* Length of change actually being executed */
/*  used for debug display */
int endoffile_in_mch[2]; /* used to flag an endoffile on the left side of a possible match
												 added 7/31/95 DAR */
int endoffile_in_match; /* used to flag an endoffile on the left side of a match
                                    added 7/31/95 DAR */
char predefinedBuffer[50];    /* ???used to resolve predefined stores  */
SSINT *store_area;  /* Store area (dynamically allocated) */ //7.4.15 BDW store_area now has file scope rather than scope within ccstart()
/* cccomp.c formerly static variable */
char keyword[20+1];         /* keyword for identification          */
bool begin_found;           /* TRUE == we have a begin statement   */
/* cccompfn.c formerly static variable */
char *precparse;            /* used by chk_prec_cmd and compilecc  */
char *lpszCCTableBuffer;    // ???

/* ccmsdo.c formerly static variable */
int org_break_flag;    // ???
bool concat;           // ???concatenated output flag
bool bAppendFlag;      // ???Append mode flag
bool bOutFileExists;   // ???TRUE is there is already a file by that name
char *cpOutMode;       // ???Records appending or writing
bool bWriteFileExists; // ???TRUE is there is already a file by that name
char *cpWriteOutMode;  // ???Records appending or writing

/* filewc.c formerly static variable */
char (*fn_arr_pnt)[ARRAY_SIZE];      /* ???array of input file names */
char (*pad_arr_pnt)[ARRAY_SIZE_2];   /* ???used to sort the above */
char inpath[PATH_LEN];               /* ???path specification from input file */
char *inpath_end;                    /* ???pointer to end of input file path */
char outpath[PATH_LEN];              /* ???path specification from output file */
char *outpath_end;                   /* ???pointer to end of output file path */
int files_read;                      /* ???number of input files to be processed */
int *gpindex;                        // ???
int ind_incr;                        /* ???index to next input file */

/* ccstate.c and other DLL formerly static variable */
int nInBuf;                 // size of the cc input buffer passed to DLL
int nInSoFar;               // number of bytes passed in so far this call
bool bPassedAllData;        // true if we have already been passed all of
// our input data from user callback routine
char *lpOutBuf;             // points to cc output buffer for DLL
char *lpOutBufStart;        // start of CC output buffer when CC back end
int nMaxOutBuf;             // maximum size of CC output buffer for DLL
int nUsedOutBuf;            // used size of CC output buffer for DLL
bool bMoreNextTime;         // TRUE if more data but filled output buffer
bool bNeedMoreInput;
bool bOutputBufferFull;
bool bFirstDLLCall;         // TRUE if first call to CCGetBuffer DLL call
bool bBeginExecuted;        // TRUE if begin statement was executed or it does not exist
const void * hInputBuffer;        // handle for CC DLL input buffer area
const void * hOutputBuffer;       // handle for CC DLL output buffer area
char *lpInBuf;              // points to cc input buffer for DLL
char *lpInBufStart;         // points to start of cc input buffer for DLL
bool bSavedDblChar;         // TRUE if saved second half of doublebyte
// char that needs to be output to the user
char dblSavedChar;          // saved second half of doublebyte character
bool bSavedInChar;          // TRUE if saved possible first half double-
// byte char from last byte of input buffer
char inSavedChar;           // saved possible first half of doublebyte
char *lpAuxBufStart;        // start of special auxiliary output buffer
char *lpAuxOutBuf;          // next write position in auxiliary buffer
char *lpAuxNextToOutput;    // next aux buffer position to be outputted
bool bAuxBufUsed;           // true if we have allocated aux buf to use
bool bProcessingDone;       // true if DLL return completion rc to user
bool bPassNoData;           // true if DLL needs no data next time called
int nCharsInAuxBuf;         // length of used (filled) part of aux buffer
int iAuxBuf;
int nAuxLength;             // length of special auxiliary output buffer
const void * hAuxBuf;             // handle for special auxiliary output buffer
const void * hStoreArea;          // handle for our store area
long lDLLUserInputCBData;   // user data passed to input callback
long lDLLUserOutputCBData;  // user data passed to output callback
long lDLLUserErrorCBData;   // user data passed to error callback
const void * hActiveCCTable;      // ???
WFILE *fUserInFile;         // ???file for input file name passed in to DLL
WFILE *fUserOutFile;        // ???file for output file name passed in to DLL
const void * hCCParent;           // handle for calling program
wxString cctpath; //char cctpath[PATH_LEN+1];   // cct file path (includes null at end)
};
#endif /* CCModule_h */
