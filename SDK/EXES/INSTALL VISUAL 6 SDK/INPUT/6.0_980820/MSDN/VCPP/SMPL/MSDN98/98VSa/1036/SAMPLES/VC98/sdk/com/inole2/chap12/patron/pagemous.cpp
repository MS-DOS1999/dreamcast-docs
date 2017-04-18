/*
 * PAGEMOUS.CPP
 * Patron Chapter 12
 *
 * Implementation of mouse-related member functions of CPage.
 * The remainder is in PAGE.CPP.  This separate file keeps this
 * grungy hit-testing/drawing code out of our way.
 *
 * Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Microsoft
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#include "patron.h"


//Lookups into the array using g_rgHTCode[x+y*3] in PAGEMOUS.CPP
#define YTOP            0
#define YMID            1
#define YBOT            2
#define XLEFT           0
#define XMID            1
#define XRIGHT          2

//Values to restrict sizing in CPage::OnMouseMove
#define SIZINGTOP       0x0001
#define SIZINGBOTTOM    0x0002
#define SIZINGLEFT      0x0004
#define SIZINGRIGHT     0x0008


//This array is for hit-testing lookups
static UINT g_rgHTCode[9]={HTTOPLEFT, HTTOP, HTTOPRIGHT
    , HTLEFT, HTCLIENT, HTRIGHT, HTBOTTOMLEFT, HTBOTTOM
    , HTBOTTOMRIGHT};


//This is for restricting tracking based on the hit-test
static UINT g_rguSizingFlags[9]={SIZINGTOP | SIZINGLEFT, SIZINGTOP
    , SIZINGTOP | SIZINGRIGHT, SIZINGLEFT, 0, SIZINGRIGHT
    , SIZINGBOTTOM | SIZINGLEFT, SIZINGBOTTOM
    , SIZINGBOTTOM | SIZINGRIGHT};



/*
 * CPage::OnLeftDown
 *
 * Purpose:
 *  Called when the user clicks with the left button on this page.
 *  We find the object under that position that is visibly on top
 *  (always the first one under this location in the page list since
 *  we paint in reverse order) and select it.
 *
 * Parameters:
 *  uKeys           UINT carrying the key state.
 *  x, y            UINT coordinates of the click in device units.
 *
 * Return Value:
 *  BOOL            Indicates if the action changed the object.
 */

BOOL CPage::OnLeftDown(UINT uKeys, UINT x, UINT y)
    {
    UINT        iTenant;
    PCTenant    pTenant;

    /*
     * If the mouse is in a position to start sizing, start
     * the debounce timer and note the condition.  The sizing
     * will start in OnTimer or OnMouseMove.  This will always
     * happen on the currently selected tenant, and m_uHTCode is
     * set in OnNCHitTest below.
     */
    if (HTNOWHERE!=m_uHTCode && HTCLIENT!=m_uHTCode)
        {
        m_fSizePending=TRUE;

        //Save down point and start timer.
        m_ptDown.x=x;
        m_ptDown.y=y;

        m_fTimer=TRUE;
        SetTimer(m_hWnd, IDTIMER_DEBOUNCE, m_cDelay, NULL);
        return FALSE;
        }

    iTenant=TenantFromPoint(x, y, &pTenant);

    if (NULL==pTenant)
        return FALSE;

    //If this one is already current, we might be now sizing.
    if (pTenant==m_pTenantCur)
        return FALSE;

    //Deselect the current tenant
    if (NULL!=m_pTenantCur)
        m_pTenantCur->Select(FALSE);

    //Move this tenant to the top of the list
    m_iTenantCur=0;

    SendMessage(m_hWndTenantList, LB_DELETESTRING, iTenant, 0L);
    SendMessage(m_hWndTenantList, LB_INSERTSTRING, 0,(LONG)pTenant);

    //Select and repaint the new tenant to show it up front
    m_pTenantCur=pTenant;

    m_pTenantCur->Repaint();
    m_pTenantCur->Select(TRUE);

    return FALSE;
    }






/*
 * CPage::OnLeftUp
 *
 * Purpose:
 *  Called when the user clicks up with the left button on this
 *  page. We stop tracking on this message, if necessary, and
 *  resize the object.
 *
 * Parameters:
 *  uKeys           UINT carrying the key state.
 *  x, y            UINT coordinates of the click in device units.
 *
 * Return Value:
 *  BOOL            Indicates if this action changed the object.
 */

BOOL CPage::OnLeftUp(UINT uKeys, UINT x, UINT y)
    {
    RECT    rc, rcT;

    if (m_fSizePending)
        {
        m_fSizePending=FALSE;

        if (m_fTimer)
            {
            KillTimer(m_hWnd, IDTIMER_DEBOUNCE);
            m_fTimer=FALSE;
            }

        return FALSE;
        }

    if (!m_fTracking)
        return FALSE;

    //Remove the dotted rectangle.
    RECTFROMRECTL(rc, m_rcl)
    DrawFocusRect(m_hDC, &rc);
    ReleaseDC(m_hWnd, m_hDC);

    ReleaseCapture();
    m_fTracking=FALSE;

    //If the original and new rects are the same, nothing happened.
    RECTFROMRECTL(rcT, m_rclOrg);

    if (EqualRect(&rc, &rcT))
        return FALSE;

    RECTFROMRECTL(rcT, m_rclOrg);
    InvalidateRect(m_hWnd, &rcT, TRUE);

    //Invalidate on the screen before accounting for scrolling
    InvalidateRect(m_hWnd, &rc, TRUE);

    //Factor in scrolling and tell the tenant where it now stands.
    OffsetRect(&rc, (int)m_pPG->m_xPos, (int)m_pPG->m_yPos);
    RECTLFROMRECT(m_rcl, rc);
    m_pTenantCur->RectSet(&m_rcl, TRUE);

    UpdateWindow(m_hWnd);
    return TRUE;
    }





/*
 * CPage::OnLeftDoubleClick
 *
 * Purpose:
 *  Called when the user double-clicks with the left button on this
 *  page.  We find the object under that position that is visibly on
 *  top (always the first one under this location in the page list
 *  since we paint in reverse order) and activate it.
 *
 * Parameters:
 *  uKeys           UINT carrying the key state.
 *  x, y            UINT coordinates of the click in device units.
 *
 * Return Value:
 *  BOOL            Indicates if the action changed the object.
 */

BOOL CPage::OnLeftDoubleClick(UINT uKeys, UINT x, UINT y)
    {
    /*
     * The current tenant is the only one that can be activated, so
     * we just have to make sure the mouse is there.  For that we
     * can use the last hit-test code we saw since it's updated on
     * every mouse move.
     */

    if (HTNOWHERE!=m_uHTCode)
        return m_pTenantCur->Activate(OLEIVERB_PRIMARY);

    return FALSE;
    }






/*
 * CPage::OnMouseMove
 *
 * Purpose:
 *  Processes WM_MOUSEMOVE on a page so we can handle tracking
 *  resize of a tenant.
 *
 * Parameters:
 *  x, y            int device coordinates to check.
 *
 * Return Value:
 *  None
 */

void CPage::OnMouseMove(UINT uKeys, int x, int y)
    {
    RECT        rc, rcO, rcB;
    int         cxy;

    if (m_fSizePending)
        {
        int     dx, dy;

        dx=(x > m_ptDown.x) ? (x-m_ptDown.x) : (m_ptDown.x-x);
        dy=(y > m_ptDown.y) ? (y-m_ptDown.y) : (m_ptDown.y-y);

        /*
         * Has the mouse moved outside the debounce distance?  If
         * so, we can start sizing.  Note that this happens
         * regardless of the timer state.
         */
        if (dx > m_cxyDist || dy > m_cxyDist)
            {
            POINT       pt;

            m_fSizePending=FALSE;

            if (m_fTimer)
                {
                KillTimer(m_hWnd, IDTIMER_DEBOUNCE);
                m_fTimer=FALSE;
                }

            StartSizeTracking();

            /*
             * Since we might have moved out of the sizing handle
             * in order to start the operation, we need to set the
             * m_uSizingFlags field based on the original down point
             * for subsequent mouse moves to function properly.
             * Note that OnNCHitTest expects screen coordinates.
             */
            SETPOINT(pt, m_ptDown.x, m_ptDown.y);
            ClientToScreen(m_hWnd, &pt);
            OnNCHitTest(pt.x, pt.y);
            OnSetCursor(m_uHTCode);
            return;
            }
        }

    if (!m_fTracking)
        return;

    //Get rid of the old rectangle.
    RECTFROMRECTL(rc, m_rcl)
    DrawFocusRect(m_hDC, &rc);

    /*
     * Calculate the new.  The flags in m_uSizingFlags tell us what
     * to change.  We limit the object by the page margins and a
     * minimum size of 3*CXYHANDLE in either dimension.
     */
    cxy=3*CXYHANDLE;

    RECTFROMRECTL(rcO, m_rclOrg);
    RECTFROMRECTL(rcB, m_rclBounds);

    if (m_uSizingFlags & SIZINGTOP)
        {
        if (y >= rcO.bottom-cxy)
            y=rcO.bottom-cxy;

        if (y <= rcB.top)           //Limit to top of page.
            y=rcB.top;

        m_rcl.top=y;
        }

    if (m_uSizingFlags & SIZINGBOTTOM)
        {
        if (y <= rcO.top+cxy)
            y=rcO.top+cxy;

        if (y >= rcB.bottom)         //Limit to bottom of page.
            y=rcB.bottom;

        m_rcl.bottom=y;
        }

    if (m_uSizingFlags & SIZINGLEFT)
        {
        if (x >= rcO.right-cxy)
            x=rcO.right-cxy;

        if (x <= rcB.left)           //Limit to left of page.
            x=rcB.left;

        m_rcl.left=x;
        }

    if (m_uSizingFlags & SIZINGRIGHT)
        {
        if (x <= rcO.left+cxy)
            x=rcO.left+cxy;

        if (x >= rcB.right)          //Limit to right of page.
            x=rcB.right;

        m_rcl.right=x;
        }


    //Draw the new
    RECTFROMRECTL(rc, m_rcl)
    DrawFocusRect(m_hDC, &rc);

    return;
    }




/*
 * CPage::OnTimer
 *
 * Purpose:
 *  Processes WM_TIMER messages to a page used to perform mouse
 *  debouncing.
 *
 * Parameters:
 *  uID             UINT timer ID.
 *
 * Return Value:
 *  None
 */

void CPage::OnTimer(UINT uID)
    {
    if (m_fSizePending)
        {
        /*
         * Having this function called means the delay requirement
         * is satisfied.  Start tracking.
         */

        m_fSizePending=FALSE;
        KillTimer(m_hWnd, IDTIMER_DEBOUNCE);
        m_fTimer=FALSE;
        StartSizeTracking();
        }

    return;
    }





/*
 * CPage::StartSizeTracking
 *
 * Purpose:
 *  Begins sizing of a tenant when mouse debounce conditions are
 *  met.
 *
 * Parameters:
 *  uID             UINT timer ID.
 *
 * Return Value:
 *  None
 */

void CPage::StartSizeTracking(void)
    {
    RECT        rc;

    m_pTenantCur->RectGet(&m_rcl, TRUE);
    SetCapture(m_hWnd);
    m_fTracking=TRUE;

    m_hDC=GetDC(m_hWnd);

    //Place the rectangle exactly where it is on the screen.
    RECTFROMRECTL(rc, m_rcl)
    OffsetRect(&rc, -(int)m_pPG->m_xPos, -(int)m_pPG->m_yPos);
    RECTLFROMRECT(m_rcl, rc);
    m_rclOrg=m_rcl;

    DrawFocusRect(m_hDC, &rc);

    m_pPG->CalcBoundingRect(&rc, TRUE);
    RECTLFROMRECT(m_rclBounds, rc);
    return;
    }





/*
 * CPage::OnNCHitTest
 *
 * Purpose:
 *  Processes WM_NCHITTEST on a page so we can check for hits on the
 *  handles of the selected object for resizing.  We only save
 *  information for ourselves and do not interfere with normal
 *  hit-testing.
 *
 * Parameters:
 *  x, y            UINT device coordinates to check.
 *
 * Return Value:
 *  None
 */

void CPage::OnNCHitTest(UINT x, UINT y)
    {
    RECT        rc;
    RECTL       rcl;
    int         iMid1, iMid2;
    int         xHit, yHit;
    POINT       pt;
    int         x0, y0;

    /*
     * Ignore this message if it occurs during tracking to adjust
     * for the behavior of oddball mouse drivers.
     */
    if (m_fSizePending || m_fTracking)
        return;

    //Default: don't start sizing on a click, don't hit an object.
    m_uSizingFlags=0;
    m_uHTCode=HTNOWHERE;

    if (NULL==m_pTenantCur)
        return;

    //Convert device points to our coordinates
    m_pTenantCur->RectGet(&rcl, FALSE);
    RECTFROMRECTL(rc, rcl);
    RectConvertMappings(&rc, NULL, TRUE);
    OffsetRect(&rc, -(int)m_pPG->m_xPos, -(int)m_pPG->m_yPos);

    SETPOINT(pt, x, y);
    ScreenToClient(m_hWnd, &pt);
    x0=pt.x;
    y0=pt.y;

    if (x0 < rc.left || x0 > rc.right)
        return;

    if (y0 < rc.top || y0 > rc.bottom)
        return;

    //It's at least in the object.
    m_uHTCode=HTCLIENT;

    //Check for hits in horizontal regions
    xHit=NOVALUE;
    iMid1=rc.left+((rc.right-rc.left-CXYHANDLE) >> 1);
    iMid2=rc.left+((rc.right-rc.left+CXYHANDLE) >> 1);

    if (x0 >= rc.left && x0 <= rc.left+CXYHANDLE)
        xHit=XLEFT;
    else if (x0 >= iMid1 && x0 <= iMid2)
        xHit=XMID;
    else if (x0 >= rc.right-CXYHANDLE && x0 <= rc.right)
        xHit=XRIGHT;

    if (NOVALUE==xHit)
        return;

    //Check for hits in vertical regions
    yHit=NOVALUE;
    iMid1=rc.top+((rc.bottom-rc.top-CXYHANDLE) >> 1);
    iMid2=rc.top+((rc.bottom-rc.top+CXYHANDLE) >> 1);

    if (y0 >= rc.top && y0 <= rc.top+CXYHANDLE)
        yHit=YTOP;
    else if (y0 >= iMid1 && y0 <= iMid2)
        yHit=YMID;
    else if (y0 >= rc.bottom-CXYHANDLE && y0 <= rc.bottom)
        yHit=YBOT;

    if (NOVALUE==yHit)
        return;

    //We hit a handle, so save our HT code
    m_uSizingFlags=g_rguSizingFlags[xHit+(yHit*3)];
    m_uHTCode=g_rgHTCode[xHit+(yHit*3)];
    return;
    }





/*
 * CPage::SetCursor
 *
 * Purpose:
 *  Processes WM_SETCURSOR using the code from OnNCHitTest.
 *
 * Parameters:
 *  x, y            UINT device coordinates to check.
 *
 * Return Value:
 *  LRESULT         HT* code for Windows.
 */

BOOL CPage::OnSetCursor(UINT uHTCode)
    {
    HCURSOR     hCur;
    UINT        iCur;

    /*
     * We really just ignore uHTCode and use the one we saved
     * in OnNCHitTest.
     */

    switch (m_uHTCode)
        {
        case HTTOP:
        case HTBOTTOM:
            iCur=IDC_VARROWS;
            break;

        case HTLEFT:
        case HTRIGHT:
            iCur=IDC_HARROWS;
            break;


        case HTTOPLEFT:
        case HTBOTTOMRIGHT:
            iCur=IDC_NWSEARROWS;
            break;

        case HTTOPRIGHT:
        case HTBOTTOMLEFT:
            iCur=IDC_NESWARROWS;
            break;

        default:
            return FALSE;
        }

    hCur=UICursorLoad(iCur);
    SetCursor(hCur);

    return TRUE;
    }





/*
 * CPage::TenantFromPoint
 * (Protected)
 *
 * Purpose:
 *  Finds the tenant under the given device coordinates on this
 *  page.
 *
 * Parmeters:
 *  x, y            UINT coordinates.
 *  ppTenant        PCTenant * in which to return the pointer.
 *
 * Return Value:
 *  UINT            Index of the matched tenant, NOVALUE if not
 *                  found.
 */

UINT CPage::TenantFromPoint(UINT x, UINT y, PCTenant *ppTenant)
    {
    PCTenant    pTenant;
    RECTL       rcl;
    UINT        i;
    int         x0, y0;

    x0=x+m_pPG->m_xPos;
    y0=y+m_pPG->m_yPos;

    for (i=0; i < m_cTenants; i++)
        {
        if (!TenantGet(i, &pTenant, FALSE))
            continue;

        pTenant->RectGet(&rcl, TRUE);

        //Essentially Perform PointInRECTL
        if (x0 >= rcl.left && x0 <= rcl.right)
            {
            if (y0 <=rcl.bottom && y0 >=rcl.top)
                {
                *ppTenant=pTenant;
                return i;
                }
            }
        }

    *ppTenant=NULL;
    return NOVALUE;
    }