/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			AdaptitConstants.h
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is a header file containing some constants used by Adapt It. 
/////////////////////////////////////////////////////////////////////////////

// constants used by Adapt It

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "AdaptitConstants.h"
#endif

#define VERSION_NUMBER		4 // version 2: from 3rd Jan 2001, flags on CSourcePhrase for start
							  // and end of a retranslation; from 14th May 2003, capacity to do
							  // and see a glossing line as well as adapting line is version 3
							  // From 23 June 2005, five BOOL members added to CSourcePhrase for
							  // support of free translations (3), notes (1) and bookmarks (1).
							  // whm Note: Since the wx version only reads xml data we are 100%
							  // compatible with the MFC version number scheme for version 4.
#define VERT_SPACE			4
#define MAX_WORDS			10	// maximum # of words allowed in a src phrase before Adapt It chokes
#define MAX_STRIPS			6000
#define MAX_PILES			36  // per strip
#define MAX_CELLS			5   // per pile (in version 2.0 and onwards, to support glossing), was 4
#define WORK_SPAN			60  // how many extra elements beyond the prec & foll context to 
								// allow for a move
#define NUM_PREWORDS		40  // how many words to allow in preceding context (max)
#define NUM_FOLLWORDS		30  // how many words to allow in the following context (max)
#define RH_SLOP				40  // leave 40 pixels of white space at end of strip, when strip-wrap
								// is on
#define MAXPUNCTPAIRS		26  //24  // maximum number of paired src & target punct chars which can be 
								// handled by the dialog
#define MAXTWOPUNCTPAIRS	10  //8
#define WIDECHARBUFSIZE		512 // big enough for anything probably, eg. CChooseTranslation, etc.

// unicode-based filepaths, and deep nesting of files with long names can override the microsoft 
// default buffer sizes (and this happened at CTC 2002 demo!), so increase this to safe limits - 
// see _MAX_PATH in STDLIB.H for the old limits (about 230 or so)
#define AI_MAX_PATH   860 /* max. length of full pathname */
#define AI_MAX_DRIVE  64   /* max. length of drive component */
#define AI_MAX_DIR    850 /* max. length of path component */
#define AI_MAX_FNAME  850 /* max. length of file name component */
#define AI_MAX_EXT    850 /* max. length of extension component */
#define MIN_FONT_SIZE	6
#define MAX_FONT_SIZE	72
#define MAX_DLG_FONT_SIZE	24
#define MIN_DLG_FONT_SIZE	11

// for free translation support - added BEW on 24Jun05, changed to ..._WORDS on 28Apr06
#define MIN_FREE_TRANS_WORDS	5
