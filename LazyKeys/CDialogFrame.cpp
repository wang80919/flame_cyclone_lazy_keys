#include "CDialogFrame.h"
#include "resource.h"
#include <Commctrl.h>
#include "Win32Utils/CPathUtils.h"
#include "Win32Utils/CInputUtils.h"
#include "Win32Utils/CStrUtils.h"
#include "Win32Utils/CProcessUtilsEx.h"

#pragma comment(lib, "Imm32.lib")

BEGIN_DLG_MESSAGE_MAP(CDialogFrame, CDialogBase)
	ON_DLG_MESSAGE(WM_INITDIALOG, &CDialogFrame::OnInitDialog)
	ON_DLG_MESSAGE(WM_CLOSE, &CDialogFrame::OnClose)
    ON_DLG_MESSAGE(WM_LBUTTONDOWN, &CDialogFrame::OnLButtonDown)
    ON_DLG_MESSAGE(WM_LBUTTONUP, &CDialogFrame::OnLButtonUp)
    ON_DLG_MESSAGE(WM_MOUSEMOVE, &CDialogFrame::OnMouseMove)
    ON_DLG_MESSAGE(WM_WINDOWPOSCHANGING, &CDialogFrame::OnWindowPosChanging)
    ON_DLG_COMMAND_NOTIFY(EN_CHANGE, IDC_HOTKEY_LAUNCH, &CDialogFrame::OnLanchNotify)
    ON_DLG_COMMAND_NOTIFY(EN_CHANGE, IDC_HOTKEY_BURST, &CDialogFrame::OnBurstNotify)
    ON_DLG_COMMAND(ID_FILE_EXIT, &CDialogFrame::OnFileExit)
    ON_DLG_COMMAND(ID_HELP_ABOUT, &CDialogFrame::OnHelpAbout)
    ON_DLG_COMMAND(IDC_CHECK_LOCK, &CDialogFrame::OnLock)
    ON_DLG_COMMAND(IDC_CHECK_KEYUP, &CDialogFrame::OnKeyUp)
	ON_DLG_COMMAND(IDC_BUTTON_LAUNCH, &CDialogFrame::OnEnable)
    ON_DLG_COMMAND(IDC_CHECK_CLASS, &CDialogFrame::OnCheckClass)
    ON_DLG_COMMAND(IDC_CHECK_CAPTION, &CDialogFrame::OnCheckCaption)
    ON_DLG_COMMAND(IDC_CHECK_PROCESS, &CDialogFrame::OnCheckProcess)
END_DLG_MESSAGE_MAP()

const int pauseTimeDelay = 0;
const int speedDelay = 50;
HHOOK g_hKbHook = NULL;

CDialogFrame* g_pDlgFrame = nullptr;

CDialogFrame::CDialogFrame()
    :
    m_wLaunchKey(0),
    m_wBurstKey(0),
    m_fCheckClass(true),
    m_fCheckCaption(true),
    m_fQuit(false),
    m_fPause(true)
{

}

CDialogFrame::~CDialogFrame()
{

}

HWND CDialogFrame::GetWindowHandle() const
{
    return m_hWnd;
}

WORD CDialogFrame::GetLaunchKey() const
{
    return m_wLaunchKey;
}

bool& CDialogFrame::GetPause()
{
    return m_fPause;
}

bool CDialogFrame::SetPause(bool fPause)
{
    bool fLast = m_fPause;
    m_fPause = fPause;
    return fLast;
}

LRESULT CDialogFrame::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
    g_pDlgFrame = this;

	// 禁用输入法
	ImmDisableIME(GetCurrentThreadId());

    // 设置图标
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
    if (hIcon)
    {
        ::SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        ::SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

	// 恢复配置
	m_cfg.Load();
    m_wLaunchKey = m_cfg.GetNumber(_T("config"), _T("key_launch"), VK_F12);
    m_wBurstKey = m_cfg.GetNumber(_T("config"), _T("key_burst"), 'F');
	::SendMessage(GetDlgItem(m_hWnd, IDC_HOTKEY_LAUNCH), HKM_SETHOTKEY, WPARAM(m_wLaunchKey), 0);
	::SendMessage(GetDlgItem(m_hWnd, IDC_HOTKEY_BURST), HKM_SETHOTKEY, WPARAM(m_wBurstKey), 0);

    m_strClass = m_cfg.GetString(_T("config"), _T("window_class"), _T("UnityWndClass"));
    SetItemString(IDC_EDIT_CLASS, m_strClass);
    SetItemString(IDC_EDIT_CAPTION, m_cfg.GetString(_T("config"), _T("window_caption"), _T("原神")));
    SetItemString(IDC_EDIT_PROCESS, m_cfg.GetString(_T("config"), _T("process_name"), _T("YuanShen.exe")));

    m_fCheckClass = m_cfg.GetNumber(_T("config"), _T("check_class"), 1);
    if (m_fCheckClass)
    {
        CheckButton(IDC_CHECK_CLASS);
    }

    m_fCheckCaption = m_cfg.GetNumber(_T("config"), _T("check_caption"), 1);
    if (m_fCheckCaption)
    {
        CheckButton(IDC_CHECK_CAPTION);
    }

    m_fCheckProcess = m_cfg.GetNumber(_T("config"), _T("check_process"), 1);
    if (m_fCheckProcess)
    {
        CheckButton(IDC_CHECK_PROCESS);
    }

    m_fLock = m_cfg.GetNumber(_T("config"), _T("key_lock"), 1);
    if (m_fLock)
    {
        CheckButton(IDC_CHECK_LOCK);
        PostCommand(IDC_CHECK_LOCK);
    }

    m_fKeyUp = m_cfg.GetNumber(_T("config"), _T("key_up"), 1);
    if (m_fKeyUp)
    {
        CheckButton(IDC_CHECK_KEYUP);
        PostCommand(IDC_CHECK_KEYUP);
    }

	// 启用键盘钩子
	g_hKbHook = ::SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeybdProc, NULL, 0);

    // 启用线程任务
    m_task = std::move(std::thread([this]() {
        InputTaskProc();
        }
    ));

	return TRUE;
}

LRESULT CDialogFrame::OnClose(WPARAM wParam, LPARAM lParam)
{
	// 保存配置
    m_cfg.SetNumber(_T("config"), _T("key_lock"), m_fLock);
    m_cfg.SetNumber(_T("config"), _T("key_up"), m_fKeyUp);
	m_cfg.SetNumber(_T("config"), _T("key_launch"), (int)m_wLaunchKey);
	m_cfg.SetNumber(_T("config"), _T("key_burst"), (int)m_wBurstKey);
    m_cfg.SetNumber(_T("config"), _T("check_class"), m_fCheckClass);
    m_cfg.SetNumber(_T("config"), _T("check_caption"), m_fCheckCaption);
    m_cfg.SetNumber(_T("config"), _T("check_process"), m_fCheckProcess);
    m_cfg.SetString(_T("config"), _T("window_class"), GetItemString(IDC_EDIT_CLASS));
    m_cfg.SetString(_T("config"), _T("window_caption"), GetItemString(IDC_EDIT_CAPTION));
    m_cfg.SetString(_T("config"), _T("process_name"), GetItemString(IDC_EDIT_PROCESS));
	m_cfg.Save();

	// 销毁钩子
	if (g_hKbHook) 
	{
		::UnhookWindowsHookEx(g_hKbHook);
		g_hKbHook = NULL;
	}

    if (m_task.joinable())
    {
        m_fQuit = true;
        m_task.join();
    }

	return (LRESULT)FALSE;
}

LRESULT CDialogFrame::OnWindowPosChanging(WPARAM wParam, LPARAM lParam)
{
    LPWINDOWPOS pWPos = (LPWINDOWPOS)lParam;
    pWPos->flags |= SWP_NOSIZE;

    return (LRESULT)FALSE;
}

LRESULT CDialogFrame::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    if (!m_fPause)
    {
        return (LRESULT)TRUE;
    }

    RECT rtTrack = { 0 };
    ::GetWindowRect(GetItem(IDC_STATIC_CAPTURE), &rtTrack);

    POINT pt = { rtTrack.left, rtTrack.top };
    ::ScreenToClient(m_hWnd, &pt);

    LONG offsetX = pt.x - rtTrack.left;
    LONG offsetY = pt.y - rtTrack.top;
    ::OffsetRect(&rtTrack, offsetX, offsetY);

    if (::PtInRect(&rtTrack, { xPos, yPos }))
    {
        m_fTrack = true;
        ::SetCapture(m_hWnd);

        HCURSOR hCursor = ::LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_CURSOR));
        ::SetCursor(hCursor);

        HICON hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_CAPTURE_YES));
        ::SendDlgItemMessage(m_hWnd, IDC_STATIC_CAPTURE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (m_fTrack)
    {
        m_fTrack = false;
        ::ReleaseCapture();
        ::SetCursor(NULL);
        HICON hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_CAPTURE_NO));
        ::SendDlgItemMessage(m_hWnd, IDC_STATIC_CAPTURE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    do
    {
        if (!m_fTrack)
        {
            break;
        }

        POINT pt = { xPos, yPos };
        ::ClientToScreen(m_hWnd, &pt);

        HWND hWnd = ::WindowFromPoint(pt);
        if (!hWnd)
        {
            break;
        }

        DWORD dwPID = 0;
        DWORD dwTID = ::GetWindowThreadProcessId(hWnd, &dwPID);
        if (::GetCurrentProcessId() == dwPID)
        {
            break;
        }

        // 获取顶级窗口句柄
        LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
        while (WS_CHILDWINDOW & style)
        {
            HWND hParent = ::GetParent(hWnd);
            if (NULL == hParent)
            {
                break;
            }

            hWnd = hParent;
            style = GetWindowLongPtr(hWnd, GWL_STYLE);
        }

        dwPID = 0;
        dwTID = ::GetWindowThreadProcessId(hWnd, &dwPID);
        if (::GetCurrentProcessId() == dwPID)
        {
            break;
        }

        TCHAR szBuf[MAX_PATH] = { 0 };
        ::GetClassName(hWnd, szBuf, _countof(szBuf));
        SetItemString(IDC_EDIT_CLASS, szBuf);
        ::GetWindowText(hWnd, szBuf, _countof(szBuf));
        SetItemString(IDC_EDIT_CAPTION, szBuf);

        m_strProcess = CProcessUtilsEx::GetProcessName(dwPID);
        SetItemString(IDC_EDIT_PROCESS, m_strProcess);
    } while (false);

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnLanchNotify(UINT nNotify, UINT nID, HWND hWnd)
{
    m_wLaunchKey = (WORD)SendItemMessage(nID, HKM_GETHOTKEY, 0, 0);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnBurstNotify(UINT nNotify, UINT nID, HWND hWnd)
{
    m_wBurstKey = (WORD)SendItemMessage(nID, HKM_GETHOTKEY, 0, 0);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnCheckClass(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd)
{
    m_fCheckClass = IsButtonChecked(nID);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnCheckCaption(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd)
{
    m_fCheckCaption = IsButtonChecked(nID);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnCheckProcess(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd)
{
    m_fCheckProcess = IsButtonChecked(nID);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnFileExit(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd)
{
    PostMessage(WM_CLOSE, 0, 0);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnHelpAbout(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd)
{
    ShellExecute(NULL, _T("open"), HELP_ABOUT_URL, NULL, NULL, SW_SHOW);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnLock(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd)
{
    m_fLock = IsButtonChecked(nID);
    EnableItem(IDC_HOTKEY_LAUNCH, !m_fLock);
    EnableItem(IDC_HOTKEY_BURST, !m_fLock);

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnKeyUp(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd)
{
    m_fKeyUp = IsButtonChecked(nID);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnEnable(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd)
{
    m_fPause = !m_fPause;

    EnableItem(IDC_CHECK_LOCK, m_fPause);
    EnableItem(IDC_CHECK_KEYUP, m_fPause);
    EnableItem(IDC_EDIT_CLASS, m_fPause);
    EnableItem(IDC_EDIT_CAPTION, m_fPause);
    EnableItem(IDC_EDIT_PROCESS, m_fPause);
    EnableItem(IDC_CHECK_CLASS, m_fPause);
    EnableItem(IDC_CHECK_CAPTION, m_fPause);
    EnableItem(IDC_CHECK_PROCESS, m_fPause);
    EnableItem(IDC_STATIC_CAPTURE, m_fPause);
    
	if (!m_fPause)
	{
		SetItemString(nID, _T("停用"));
        if (!m_fLock)
        {
            EnableItem(IDC_HOTKEY_LAUNCH, false);
            EnableItem(IDC_HOTKEY_BURST, false);
        }
	}
	else
	{
		SetItemString(nID, _T("启用"));
        if (!m_fLock)
        {
            EnableItem(IDC_HOTKEY_LAUNCH, true);
            EnableItem(IDC_HOTKEY_BURST, true);
        }
	}

    // 更新过滤配置
    m_strClass = GetItemString(IDC_EDIT_CLASS);
    m_strCaption = GetItemString(IDC_EDIT_CAPTION);
    m_strProcess = GetItemString(IDC_EDIT_PROCESS);

	return (LRESULT)TRUE;
}

// 低级键盘钩子过程
LRESULT CALLBACK CDialogFrame::LowLevelKeybdProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        KBDLLHOOKSTRUCT& event = *(PKBDLLHOOKSTRUCT)lParam;  // For convenience, maintainability, and possibly performance.
        bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool bShitf = GetAsyncKeyState(VK_SHIFT) & 0x8000;
        bool bCtrl = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        bool bAlt = GetAsyncKeyState(VK_MENU) & 0x8000;

        typedef struct _KEY_FLAGS
        {
            DWORD Value;
            union
            {
                DWORD fExtended : 1;        // 指定键是扩展键，例如功能键还是数字键盘上的键。 如果键是扩展键，则值为 1;否则为 0
                DWORD fLLInjected : 1;      // 指指定事件是否是从在较低完整性级别运行的进程中注入的。 如果出现这种情况，则值为 1;否则为 0。 请注意，每当设置位 1 时，也会设置位 4。
                DWORD Reserved1 : 2;        // 保留
                DWORD fInjected : 1;        // 指定是否注入事件。 如果出现这种情况，则值为 1;否则为 0。 请注意，设置第 4 位时不一定设置位 1。
                DWORD fContext : 1;         // 上下文代码。 如果按下 Alt 键，则值为 1;否则为 0
                DWORD Reserved2 : 1;        // 保留
                DWORD fTransition : 1;      // 转换状态。 如果按下键，则值为 0;如果释放键，则值为 1
            };
        }KEY_FLAGS;

        KEY_FLAGS flags = { 0 };
        flags.Value = event.flags;

        WORD wBurstKey = g_pDlgFrame->GetLaunchKey();
        BYTE Modifier = HIBYTE(wBurstKey);
        BYTE VkCode = LOBYTE(wBurstKey);

        if (keyDown && VkCode == event.vkCode)
        {
            do
            {
                if (bCtrl != (HOTKEYF_CONTROL == (Modifier & HOTKEYF_CONTROL))) break;
                if (bShitf != (HOTKEYF_SHIFT == (Modifier & HOTKEYF_SHIFT))) break;
                if (bAlt != (HOTKEYF_ALT == (Modifier & HOTKEYF_ALT))) break;

                g_pDlgFrame->PostMessage(WM_COMMAND, IDC_BUTTON_LAUNCH, 0);
                return 1;

            } while (false);
        }
    }

    return CallNextHookEx(g_hKbHook, nCode, wParam, lParam);
}

void CDialogFrame::InputTaskProc()
{
    auto cbInput = [this](bool bDown = true, bool bUp = true)
        {
            // 按下修饰键
            BYTE Modifier = HIBYTE(m_wBurstKey);
            BYTE VkCode = LOBYTE(m_wBurstKey);
            if (Modifier & HOTKEYF_CONTROL) CInputUtils::KeyboardByName(_T("LCtrl"), true, false);
            if (Modifier & HOTKEYF_SHIFT) CInputUtils::KeyboardByName(_T("LShift"), true, false);
            if (Modifier & HOTKEYF_ALT) CInputUtils::KeyboardByName(_T("LAlt"), true, false);

            // 按下非修饰键
            if (VkCode)
            {
                CInputUtils::KeyboardByVkCode(VkCode, false, bDown, bUp);
            }

            // 释放修饰键
            if (Modifier & HOTKEYF_ALT) CInputUtils::KeyboardByName(_T("LAlt"), false, true);
            if (Modifier & HOTKEYF_SHIFT) CInputUtils::KeyboardByName(_T("LShift"), false, true);
            if (Modifier & HOTKEYF_CONTROL) CInputUtils::KeyboardByName(_T("LCtrl"), false, true);
        };

    bool fLastState = m_fPause;
    bool fLastCheckHit = false;

    while (!m_fQuit)
    {
        if (!m_fPause)
        {
            HWND hWnd = ::GetForegroundWindow();
            bool fCheckHit = false;

            do
            {
                DWORD dwPID = 0;
                DWORD dwTID = ::GetWindowThreadProcessId(hWnd, &dwPID);
                if (::GetCurrentProcessId() == dwPID)
                {
                    break;
                }

                TCHAR szClass[MAX_PATH];
                TCHAR szCaption[MAX_PATH];
                ::GetClassName(hWnd, szClass, _countof(szClass));
                ::GetWindowText(hWnd, szCaption, _countof(szCaption));

                _tstring strProcess = CProcessUtilsEx::GetProcessName(dwPID);
                _tstring strClass = szClass;
                _tstring strCaption = szCaption;

                bool fClass = 0 == CStrUtils::CompareNoCase(m_strClass, strClass);
                bool fCaption = 0 == CStrUtils::CompareNoCase(m_strCaption, strCaption);
                bool fProcess = 0 == CStrUtils::CompareNoCase(m_strProcess, strProcess);
                bool fOnlyClass = m_fCheckClass && !m_fCheckCaption && !m_fCheckProcess;
                bool fOnlyCaption = !m_fCheckClass && m_fCheckCaption && !m_fCheckProcess;
                bool fOnlyProcess = !m_fCheckClass && !m_fCheckCaption && m_fCheckProcess;
                bool fOnlyClassCaption = m_fCheckClass && m_fCheckCaption && !m_fCheckProcess;
                bool fOnlyClassProcess = m_fCheckClass && !m_fCheckCaption && m_fCheckProcess;
                bool fOnlyCaptionProcess = !m_fCheckClass && m_fCheckCaption && m_fCheckProcess;

                // 不过滤
                if (!m_fCheckClass && !m_fCheckCaption && !m_fCheckProcess)
                {
                    fCheckHit = true;
                    cbInput(true, m_fKeyUp);
                }
                // 全过滤
                else if (m_fCheckClass && m_fCheckCaption && m_fCheckProcess)
                {
                    if (fClass && fCaption && fProcess)
                    {
                        fCheckHit = true;
                        cbInput(true, m_fKeyUp);
                    }
                }
                // 仅单项
                else if ((fOnlyClass && (fClass)) ||
                    (fOnlyCaption && (fCaption)) ||
                    (fOnlyProcess && (fProcess))
                    )
                {
                    fCheckHit = true;
                    cbInput(true, m_fKeyUp);
                }
                // 仅2项
                else if ((fOnlyClassCaption && (fClass && fCaption)) ||
                    (fOnlyClassProcess && (fClass && fProcess)) ||
                    (fOnlyCaptionProcess && (fCaption && fProcess))
                    )
                {
                    fCheckHit = true;
                    cbInput(true, m_fKeyUp);
                }

            } while (false);

            if (fLastCheckHit != fCheckHit && fLastCheckHit)
            {
                cbInput(false, true);
            }

            fLastCheckHit = fCheckHit;
        }

        if (fLastState != m_fPause && fLastState)
        {
            cbInput(false, true);
        }

        fLastState = m_fPause;
        Sleep(speedDelay);
    }
}