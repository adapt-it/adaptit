/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			stack.cpp
/// \author			Bruce Waters; modified by Bill Martin for the WX version
/// \date_created	October 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CStack class.
/// Note: my CStack class, the XML parser, and my string class CBString
/// will all be single-byte characters. When used with the Uncode supporting
/// app, these single-byte characters will be interpretted as UTF-8, and the
/// callback functions which the XML parser uses will internally convert the
/// byte string to UTF-16; and so the signature of these callbacks has to have
/// a wxString so as to accept the UTF-16 for a unicode build. Elsewhere
/// in the XML module, where strings are required, I will use my CBString class.
/// \derivation		The CStack class is not a derived class.
/////////////////////////////////////////////////////////////////////////////

// **** The following conditional compile directives added for wxWidgets support
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Stack.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include "Adapt_It.h"
#include "Stack.h"

CStack::CStack()
{
	top = 0;
	memset((void*)items,0,MAX*32);
}

bool CStack::IsEmpty() const
{
	return top == 0;
}

bool CStack::IsFull() const
{
	return top == MAX;
}

bool CStack::Push(Item item)
{
	if (top < MAX)
	{
		strcpy(items[top++],item);
		return true;
	}
	else
	{
		return false;
	}
}

bool CStack::Pop(Item item)
{
	if (top > 0)
	{
		strcpy(item,items[--top]);
		memset(items[top],0,32);
		return true;
	}
	else
	{
		return false;
	}
}

void CStack::GetTop(Item item)
{
	if (top > 0)
	{
		strcpy(item,items[--top]);
		top++;
	}
	else
	{
		int i;
		for (i=0; i<32; i++)
		{
			item[i] = '\0';
		}
	}
}

bool CStack::Contains(const Item item)
{
	if (top > 0)
	{
		int i;
		for (i = 0; i < top; i++) // BEW 5Jun10, < not <= as top is index of next 
								  // fillable cell (& it doesn't have any data in it yet)
		{
			if (strcmp(item,items[i]) == 0)
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		return false;
	}
}

// checks from 1 to 3 parents when an element is nested, starting from the one immediately
// under the top element, and comparing nThisMany - up to a maximum of 3 parents; return
// true if the ones checked are matches for what is on the stack; pass an empty string for
// any which are not to be checked - fill the parameters from the left in reverse order of
// the nesting in the xml file, item_up1 (being the first parent) must always have something
// in it if the function result is to have any useful meaning; the second parent (the more
// 'outer' tag of the nesting) would be item_up2. Usually that's all we require. For
// flexibility a third parameter is provided, for a more 'outer' still tag, but our LIFT
// support will not need to test any more than two parents. The tags tested can be at any
// depth in the stack - since the testing starts at the tag one-down from the topmost item
// in the stack. Note: the top index for the stack indexes not the top element, but the
// empty cell immediately beyond that. So if the cell with index 5 has a tagname in it,
// the the value of top would be 6 (stack indices start from 0).
// So if we have the following:
// <sense>
//     <definition>
//         <text>somePCDATA</text>
// Then the "current" top item on the stack is the one which we presumably are processing
// using, say, an AtLIFTPCDATA() or similar callback, the first parent will be <definition>
// and the second parent will be <sense> -- so we need to test only for two parents here, 
// though there may well be other tags higher up which we don't test for.
// Any arguments unused for testing in the function should have emptyStr as their parameter,
// as defined above..
// So we would make the statements:
// static char emptyStr[32];  // near the .cpp file's start
// void* r = memset((void*)emptyStr,0,32); // after the above line
// bool bMatched = pStack->MyParentsAre(2,xml_definition,xml_sense,emptyStr); // in AtXXXyyy()
// Note, the arguments from left to right correspond to moving back though nested xml tags
// starting from the one above the current one.
// 
bool CStack::MyParentsAre(int nThisMany,const Item item_up1, const Item item_up2, 
						  const Item item_up3)
{
	int index;
	int start = top - 2;
	int end = start - nThisMany + 1;
	if (nThisMany > 3) return false; // too many checks requested
	if (nThisMany >= top) return false; // can't make all the requested checks
	int counter = -1;
	// next 3 lines for debugging - comment out when verified robust
	//CBString parent1;
	//CBString parent2;
	//CBString parent3;
	for (index = start; index >= end; index--)
	{
		++counter;
		switch (counter)
		{
		case 0:
			{
				/* for debugging */ //parent1 = items[index];
				if (strcmp(item_up1,items[index]) != 0)
					return false;
			}
			break;
		case 1:
			{
				/* for debugging */ //parent2 = items[index];
				if (item_up2[0] == '\0')
					return true;
				else if (strcmp(item_up2,items[index]) != 0)
					return false;
			}
			break;
		case 2:
			{
				/* for debugging */ //parent3 = items[index];
				if (item_up3[0] == '\0')
					return true;
				else if (strcmp(item_up3,items[index]) != 0)
					return false;
			}
			break;
		}
	}
	return true;
}

