/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			stack.cpp
/// \author			Bruce Waters; modified by Bill Martin for AdaptItWX
/// \date_created	October 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CStack class.
/// Note: my CStack class, the XML parser, and my string class CBString
/// will all be single-byte characters. When used with the Uncode supporting
/// app, these single-byte characters will be interpretted as UTF-8, and the
/// callback functions which the XML parser uses will internally convert the
/// byte string to UTF-16; and so the signature of these callbacks has to have
/// a MFC CString so as to accept the UTF-16 for a unicode build. Elsewhere
/// in the XML module, where strings are required, I will use my CBString class.
/// \derivation		The CStack class is not a derived class.
/////////////////////////////////////////////////////////////////////////////

// **** The following conditional compile directives added for wxWidgets support
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "BString.h"
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

#include "Stack.h"
#include "Adapt_It.h"


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
		UInt16 oldIndex = top;
		strcpy(item,items[--top]);
		memset(items[oldIndex],0,32);
		return true;
	}
	else
	{
		return false;
	}
}
