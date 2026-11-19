#pragma once
#include <thread>
#include "CWindow/CDialogBase.h"
#include "Win32Utils/CIniHelperEx.h"

#define HELP_ABOUT_URL _T("https://gitee.com/flame_cyclone/lazy_keys")

class CDialogFrame:
	public CDialogBase
{
public:
	CDialogFrame();
	~CDialogFrame();

	WORD GetLaunchKey() const;
	bool& GetPause();
	bool SetPause(bool fPause);
	HWND GetWindowHandle() const;

public:
	dlg_msg LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
	dlg_msg LRESULT OnClose(WPARAM wParam, LPARAM lParam);
	dlg_msg LRESULT OnLButtonDown(WPARAM wParam, LPARAM lParam);
	dlg_msg LRESULT OnLButtonUp(WPARAM wParam, LPARAM lParam);
	dlg_msg LRESULT OnMouseMove(WPARAM wParam, LPARAM lParam);
	dlg_msg LRESULT OnWindowPosChanging(WPARAM wParam, LPARAM lParam);

	dlg_command LRESULT OnLanchNotify(UINT nNotify, UINT nID, HWND hWnd);
	dlg_command LRESULT OnBurstNotify(UINT nNotify, UINT nID, HWND hWnd);
	dlg_msg LRESULT OnCheckClass(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd);
	dlg_msg LRESULT OnCheckCaption(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd);
	dlg_msg LRESULT OnCheckProcess(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd);
	dlg_msg LRESULT OnLock(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd);
	dlg_msg LRESULT OnKeyUp(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd);
	dlg_msg LRESULT OnEnable(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd);
	dlg_msg LRESULT OnFileExit(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd);
	dlg_msg LRESULT OnHelpAbout(UINT nNotify, UINT nID, HWND hWnd, UINT nStart, UINT nEnd);

	DECLARE_DLG_MESSAGE_MAP()

private:

	static LRESULT CALLBACK LowLevelKeybdProc(int nCode, WPARAM wParam, LPARAM lParam);
	void InputTaskProc();

private:

	CIniHelperEx		m_cfg;
	std::thread			m_task;
	_tstring			m_strClass;
	_tstring			m_strCaption;
	_tstring			m_strProcess;
	WORD				m_wLaunchKey;
	WORD				m_wBurstKey;
	bool				m_fCheckClass;
	bool				m_fCheckCaption;
	bool				m_fCheckProcess;
	bool				m_fQuit;
	bool				m_fPause;
	bool				m_fTrack;
	bool				m_fLock;
	bool				m_fKeyUp;
};