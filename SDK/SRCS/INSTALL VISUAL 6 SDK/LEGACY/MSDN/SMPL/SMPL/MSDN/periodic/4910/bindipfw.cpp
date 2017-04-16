// bindipfw.cpp : implementation of the Binder-aware DocObject
//	in-place frame window 
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.
//

#include "stdafx.h"
#include <afxpriv.h>	 // for AfxRepositionWindow
#include "afxbind.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDocObjectIPFrameWnd

IMPLEMENT_DYNCREATE(CDocObjectIPFrameWnd, COleIPFrameWnd)

BEGIN_MESSAGE_MAP(CDocObjectIPFrameWnd, COleIPFrameWnd)
	//{{AFX_MSG_MAP(CDocObjectIPFrameWnd)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CDocObjectIPFrameWnd::CDocObjectIPFrameWnd()
{
	m_hMenuHelpPopup = NULL;
}

CDocObjectIPFrameWnd::~CDocObjectIPFrameWnd()
{
}

void CDocObjectIPFrameWnd::OnRequestPositionChange(LPCRECT lpRect)
{
	CDocObjectServerDoc* pDoc = (CDocObjectServerDoc*)GetActiveDocument();
	ASSERT_VALID(pDoc);
	ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CDocObjectServerDoc)));

	// DocObjects don't need to generate OnPosRectChange calls, so we 
	// just return if this is a DocObject.
	if (pDoc->IsDocObject())
		return;

	// The default behavior is to not affect the extent during the
	//  call to RequestPositionChange.  This results in consistent
	//  scaling behavior.
	pDoc->RequestPositionChange(lpRect);
}

void CDocObjectIPFrameWnd::RecalcLayout(BOOL /*bNotify*/)
{
	ASSERT_VALID(this);

	CDocObjectServerDoc* pDoc = (CDocObjectServerDoc*)GetActiveDocument();
	if (pDoc)
	{
		ASSERT_VALID(pDoc);
		ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CDocObjectServerDoc)));
	}

	// Normal in-place objects put scrollbars on the outside,
	//	DocObjects put them inside
	UINT nAdjustType = (pDoc && pDoc->IsDocObject())
								? CWnd::adjustBorder : CWnd::adjustOutside;

	// better have a parent window (only used for inplace)
	CWnd* pParentWnd = GetParent();
	ASSERT_VALID(pParentWnd);

	// first call reposition bars with arbitarily large rect just to
	//  see how much space the bars will take up
	CRect rectBig(0, 0, INT_MAX/2, INT_MAX/2);
	CRect rectLeft;
	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery,
		&rectLeft, &rectBig);

	// grow the rect by the size of the control bars
	CRect rect = m_rectPos;
	rect.left -= rectLeft.left;
	rect.top -= rectLeft.top;
	rect.right += INT_MAX/2 - rectLeft.right;
	rect.bottom += INT_MAX/2 - rectLeft.bottom;

	// see how much extra space for non-client areas (such as scrollbars)
	//  that the view needs.
	CWnd* pLeftOver = GetDlgItem(AFX_IDW_PANE_FIRST);
	if (pLeftOver != NULL)
	{
		rectBig = m_rectPos;
		pLeftOver->CalcWindowRect(&rectBig, nAdjustType);
		rect.left -= m_rectPos.left - rectBig.left;
		rect.top -= m_rectPos.top - rectBig.top;
		rect.right += rectBig.right - m_rectPos.right;
		rect.bottom += rectBig.bottom - m_rectPos.bottom;
	}

	// adjust for non-client area on the frame window
	CalcWindowRect(&rect, nAdjustType);

	// the frame window must be clipped to the visible part in the container
	CRect rectVis;
	rectVis.IntersectRect(&rect, &m_rectClip);

	// move the window
	AfxRepositionWindow(NULL, m_hWnd, &rectVis);

	// now resize the control bars relative to the (now moved) frame
	pParentWnd->ClientToScreen(&rect);
	ScreenToClient(&rect);
	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST,
		CWnd::reposDefault, NULL, &rect);
}

BOOL CDocObjectIPFrameWnd::BuildSharedMenu()
{
	HMENU hMenu = GetInPlaceMenu();

	// create shared menu
	ASSERT(m_hSharedMenu == NULL);
	if ((m_hSharedMenu = ::CreateMenu()) == NULL)
		return FALSE;

	// start out by getting menu from container
	memset(&m_menuWidths, 0, sizeof m_menuWidths);
	if (m_lpFrame->InsertMenus(m_hSharedMenu, &m_menuWidths) != NOERROR)
	{
		::DestroyMenu(m_hSharedMenu);
		m_hSharedMenu = NULL;
		return FALSE;
	}

	// container shouldn't touch these
	ASSERT(m_menuWidths.width[1] == 0);
	ASSERT(m_menuWidths.width[3] == 0);

	// container shouldn't touch this unless we're working with a DocObject
	CDocObjectServerDoc* pDoc = (CDocObjectServerDoc*)GetActiveDocument();
	ASSERT_VALID(pDoc);
	ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CDocObjectServerDoc)));
	if (!pDoc->IsDocObject())
		ASSERT(m_menuWidths.width[5] == 0);

	// only copy the popups if there is a menu loaded
	if (hMenu == NULL)
		return TRUE;

	// insert our menu popups amongst the container menus
	m_hMenuHelpPopup = MfcBinderMergeMenus(CMenu::FromHandle(m_hSharedMenu),
								 CMenu::FromHandle(hMenu), &m_menuWidths.width[0], 1);

	// finally create the special OLE menu descriptor
	m_hOleMenu = ::OleCreateMenuDescriptor(m_hSharedMenu, &m_menuWidths);

	return m_hOleMenu != NULL;
}

void CDocObjectIPFrameWnd::DestroySharedMenu()
{
	if (m_hSharedMenu == NULL)
	{
		ASSERT(m_hOleMenu == NULL);
		ASSERT(m_hMenuHelpPopup == NULL);
		return;
	}

	// get in-place menu to be unmerged (must be same as during activation)
	HMENU hMenu = GetInPlaceMenu();
	if (hMenu == NULL)
		return;

	// remove our menu popups from the shared menu
	MfcBinderUnmergeMenus(CMenu::FromHandle(m_hSharedMenu), 
							  CMenu::FromHandle(hMenu),
							  CMenu::FromHandle(m_hMenuHelpPopup));

	// allow container to remove its items from the menu
	ASSERT(m_lpFrame != NULL);
	VERIFY(m_lpFrame->RemoveMenus(m_hSharedMenu) == NOERROR);

	// now destroy the menu
	::DestroyMenu(m_hSharedMenu);
	m_hSharedMenu = NULL;
	if (m_hOleMenu != NULL)
	{
		VERIFY(::OleDestroyMenuDescriptor(m_hOleMenu) == NOERROR);
		m_hOleMenu = NULL;
	}
	m_hMenuHelpPopup = NULL;
}

#ifdef _DEBUG
void CDocObjectIPFrameWnd::AssertValid() const
{
	COleIPFrameWnd::AssertValid();
}

void CDocObjectIPFrameWnd::Dump(CDumpContext& dc) const
{
	COleIPFrameWnd::Dump(dc);
}
#endif //_DEBUG

