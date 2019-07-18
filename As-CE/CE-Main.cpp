#include "resource.h"
#include "MemHandle.h"

//开启视觉效果
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")  

MemHandle g_Mem;

//窗口过程
INT_PTR CALLBACK TheDialogProc(HWND,UINT,WPARAM,LPARAM);
//初始化事件
void OnInitiailze(HWND);
//控件消息事件
void OnNotify(HWND, WPARAM, LPARAM);
//控件消息事件
void OnCommand(HWND, WPARAM, LPARAM);

//更新进程列表
void UpdateProcess(HWND);
//双击指定进程
void DBlickProcess(HWND);
//读取进程内存区域
void ReadProcessMemRegion(HWND);
//第一次内存查找
void FirstMemorySearch(HWND);
//下一次内存查找
void NextMemorySearch(HWND);
//重置查找
void ReScanSearch(HWND);
//修改内存中的数值
void HackMemoryValue(HWND);
//加入地址信息
void Thread_Address(HWND);
//定时更新容器里面的地址的数值
void Thread_Update(HWND);


int WINAPI WinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	HWND hWnd = CreateDialogParamA(hInstance, MAKEINTRESOURCE(IDD_CE), 0, TheDialogProc,0);
	if (!hWnd)
		return -1;
	ShowWindow(hWnd, SW_NORMAL);
	UpdateWindow(hWnd);
	std::thread cThread(Thread_Update, hWnd);
	cThread.detach();
	MSG stMsg;
	while (GetMessageA(&stMsg, 0, 0, 0))
	{
		TranslateMessage(&stMsg);
		DispatchMessageA(&stMsg);
	}
	return static_cast<int>(stMsg.wParam);
}


INT_PTR CALLBACK TheDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		OnInitiailze(hDlg);
		return true;
	case WM_NOTIFY:
		OnNotify(hDlg, wParam, lParam);
		break;
	case WM_COMMAND:
		OnCommand(hDlg, wParam, lParam);
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return false;
}

void OnInitiailze(HWND hDlg)
{
	//更新进程列表
	UpdateProcess(hDlg);
	//获取视图句柄
	HWND hViewAddress = GetDlgItem(hDlg, IDC_LIST_ADDRESS);
	//设置为报表模式
	ListView_SetView(hViewAddress, LVS_REPORT);
	//设置扩展样式
	ListView_SetExtendedListViewStyle(hViewAddress, LVS_EX_AUTOAUTOARRANGE| LVS_EX_AUTOSIZECOLUMNS| LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER| LVS_EX_GRIDLINES);
	//添加Column信息
	LVCOLUMNA stColumn;
	memset(&stColumn,0,sizeof(LVCOLUMNA));
	stColumn.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
	stColumn.fmt = LVCFMT_LEFT;
	stColumn.cx = 50;
	stColumn.iSubItem = 0;
	stColumn.pszText = (LPSTR)"Index";
	ListView_InsertColumn(hViewAddress, 0, &stColumn);
	stColumn.cx = 100;
	stColumn.iSubItem = 1;
	stColumn.pszText = (LPSTR)"Address";
	ListView_InsertColumn(hViewAddress, 1, &stColumn);
	stColumn.iSubItem = 2;
	stColumn.pszText = (LPSTR)"Target Value";
	ListView_InsertColumn(hViewAddress, 2, &stColumn);
	stColumn.iSubItem = 3;
	stColumn.pszText = (LPSTR)"Current Value";
	ListView_InsertColumn(hViewAddress, 3, &stColumn);
	stColumn.cx = 300;
	stColumn.iSubItem = 4;
	stColumn.pszText = (LPSTR)"Note";
	ListView_InsertColumn(hViewAddress, 4, &stColumn);
	//设置按钮的状态
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_NEXT), false);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_HACK), false);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_RESCAN), false);
}

void OnNotify(HWND hDlg, WPARAM wParam,LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_LIST_ADDRESS)
	{
		if (((LPNMHDR)lParam)->code == NM_CLICK)//单击
		{
			//获取地址
			char szBuffer[20];
			ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_ADDRESS),
				((LPNMITEMACTIVATE)lParam)->iItem, 1, szBuffer, 20);
			Edit_SetText(GetDlgItem(hDlg, IDC_EDIT_TIPS), szBuffer);
		}
		else if (((LPNMHDR)lParam)->code == NM_DBLCLK)//双击
		{
			//这里想实现在列表视图中，双击就能就地编辑note内存数值的说明文本，基本思路就是双击的时候在指定note上面创建一个Edit控件，编辑完成后获取文本再删除Edit控件，将文本放到指定的note里面。
			//RECT stRect;
			//ListView_GetItemRect(GetDlgItem(hDlg, IDC_LIST_ADDRESS), ((LPNMITEMACTIVATE)lParam)->iItem,
			//	&stRect, LVIR_LABEL);

			char szBuffer[20];
			ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_ADDRESS),
				((LPNMITEMACTIVATE)lParam)->iItem, 1, szBuffer, 20);
			Edit_SetText(GetDlgItem(hDlg, IDC_EDIT_TIPS), szBuffer);
		}
	}
}

void OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	//这里处理双击进程列表的指定进程的
	if (LOWORD(wParam) == IDC_LIST_PROCESS && HIWORD(wParam) == LBN_DBLCLK)
	{
		DBlickProcess(hDlg);
		return;
	}
	//按钮判断
	switch (LOWORD(wParam))
	{
		case IDC_BUTTON_UPDATEPROCESS:
			UpdateProcess(hDlg);
			break;
		case IDC_BUTTON_OPEN:
			ReadProcessMemRegion(hDlg);
			break;
		case IDC_BUTTON_FIRST:
			FirstMemorySearch(hDlg);
			break;
		case IDC_BUTTON_NEXT:
			NextMemorySearch(hDlg);
			break;
		case IDC_BUTTON_RESCAN:
			ReScanSearch(hDlg);
			break;
		case IDC_BUTTON_HACK:
			HackMemoryValue(hDlg);
			break;
	}
}

void UpdateProcess(HWND hDlg)
{
	//获取进程列表句柄
	HWND hListProcess = GetDlgItem(hDlg, IDC_LIST_PROCESS);
	//清空进程列表
	ListBox_ResetContent(hListProcess);
	//创建进程遍历
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!hSnap)
		return;
	char szFormat[200];
	PROCESSENTRY32 stProcInfo;
	memset(&stProcInfo, 0, sizeof(PROCESSENTRY32));
	stProcInfo.dwSize = sizeof(PROCESSENTRY32);
	//查找第一个进程消息
	bool bRet = Process32First(hSnap, &stProcInfo);
	while (bRet)
	{
		//格式化字符串
		wsprintfA(szFormat, "%d     [%s]", 
			stProcInfo.th32ProcessID, stProcInfo.szExeFile);
		//添加到进程列表
		ListBox_AddString(hListProcess, szFormat);
		//查找下一个进程消息
		bRet = Process32Next(hSnap, &stProcInfo);
	}
	CloseHandle(hSnap);
}

void DBlickProcess(HWND hDlg)
{
	//获取进程列表句柄
	HWND hListProcess = GetDlgItem(hDlg, IDC_LIST_PROCESS);
	//获取进程列表的双击项序号
	int nProcessIndex = ListBox_GetCurSel(hListProcess);
	char szBuffer[200];
	//获取进程信息
	ListBox_GetText(hListProcess, nProcessIndex, szBuffer);
	//获取目标进程编辑框句柄
	HWND hEditTarget = GetDlgItem(hDlg, IDC_EDIT_TARGETPROCESS);
	//设置目标进程编辑框
	Edit_SetText(hEditTarget, szBuffer);
}

void ReadProcessMemRegion(HWND hDlg)
{
	//获取目标进程编辑框句柄
	HWND hEditProcess = GetDlgItem(hDlg, IDC_EDIT_TARGETPROCESS);
	//获取指定进程
	char szProcess[200];
	Edit_GetText(hEditProcess, szProcess, 200);
	//获取进程ID
	int nPID = 0;
	sscanf(szProcess, "%d", &nPID);
	//获取提示栏的句柄
	HWND hEditTips = GetDlgItem(hDlg, IDC_EDIT_TIPS);
	//读取进程内存区域
	if (g_Mem.ReadMemoryRegion(nPID))
		Edit_SetText(hEditTips, "[Tips]: Read memory regioin success");
	else
		Edit_SetText(hEditTips, "[Tips]: Read memory regioin fail");
}

void FirstMemorySearch(HWND hDlg)
{
	//获取整形数值
	int nValue = GetDlgItemInt(hDlg, IDC_EDIT_VALUE, 0, 0);
	//获取提示栏的句柄
	HWND hEditTips = GetDlgItem(hDlg, IDC_EDIT_TIPS);
	//查找内存数值
	if (g_Mem.FirstFindMemory(nValue))
	{
		EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_NEXT),true);
		EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_HACK), true);
		EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_RESCAN), true);
		EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_FIRST), false);
		Edit_SetText(hEditTips, "[Tips]: First search memory value success");
	}
	else
		Edit_SetText(hEditTips, "[Tips]: First search memory value fail");
	//创建一个线程显示地址信息
	std::thread cAdd(Thread_Address, GetDlgItem(hDlg, IDC_LIST_ADDRESS));
	cAdd.detach();
}

void NextMemorySearch(HWND hDlg)
{
	//获取整形数值
	int nValue = GetDlgItemInt(hDlg, IDC_EDIT_VALUE, 0, 0);
	//获取提示栏的句柄
	HWND hEditTips = GetDlgItem(hDlg, IDC_EDIT_TIPS);
	if (g_Mem.NextFindMemory(nValue))
		Edit_SetText(hEditTips, "[Tips]: Next search memory value success");
	else
		Edit_SetText(hEditTips, "[Tips]: Next search memory value success");
	//创建一个线程显示地址信息
	std::thread cAdd(Thread_Address, GetDlgItem(hDlg, IDC_LIST_ADDRESS));
	cAdd.detach();
}

void ReScanSearch(HWND hDlg)
{
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_NEXT), false);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_HACK), false);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_RESCAN), false);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_FIRST), true);
	std::unique_lock<std::mutex> cLock(g_Mem.m_Mutex, std::defer_lock);
	cLock.lock();
	g_Mem.m_ListAddress.clear();
	cLock.unlock();
	ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_ADDRESS));
}

void HackMemoryValue(HWND hDlg)
{
	//获取数值
	int nValue = GetDlgItemInt(hDlg, IDC_EDIT_NODIFYVALUE, 0, 0);
	//获取提示框的句柄
	HWND hEditTips = GetDlgItem(hDlg, IDC_EDIT_TIPS);
	//获取地址
	char szAddress[30];
	Edit_GetText(hEditTips, szAddress, 30);
	//将地址进行转化
	int nAddress;
	sscanf(&szAddress[2], "%x", &nAddress);
	//修改内存数值
	if (g_Mem.NodifyMemory(nAddress, nValue))
		Edit_SetText(hEditTips, "[Tips]: Hack success");
	else
		Edit_SetText(hEditTips, "[Tips]: Hack fail");
}

void Thread_Address(HWND hListViewAddress)
{
	//将原来的数据全部清除
	ListView_DeleteAllItems(hListViewAddress);
	char szBuffer[50];
	LVITEMA stItem;
	memset(&stItem,0,sizeof(LVITEMA));
	stItem.mask = LVIF_TEXT | LVCF_SUBITEM;
	int nIndex = 0;
	std::unique_lock<std::mutex> cLock(g_Mem.m_Mutex, std::defer_lock);
	cLock.lock();
	for (std::list<AddressInfo>::iterator it = g_Mem.m_ListAddress.begin();
		it != g_Mem.m_ListAddress.end(); it++,nIndex++)
	{
		stItem.iItem = nIndex;
		wsprintfA(szBuffer, "%d", nIndex);
		stItem.pszText = szBuffer;
		stItem.iSubItem = 0;
		ListView_InsertItem(hListViewAddress, &stItem);
		wsprintfA(szBuffer, "0x%x", it->nAddress);
		stItem.pszText = szBuffer;
		stItem.iSubItem = 1;
		ListView_SetItem(hListViewAddress, &stItem);
		wsprintfA(szBuffer, "%ld", it->nTargetValue);
		stItem.pszText = szBuffer;
		stItem.iSubItem = 2;
		ListView_SetItem(hListViewAddress, &stItem);
		wsprintfA(szBuffer, "%ld", it->nCurrentValue);
		stItem.pszText = szBuffer;
		stItem.iSubItem = 3;
		ListView_SetItem(hListViewAddress, &stItem);
		stItem.pszText = it->szNote;
		stItem.iSubItem = 4;
		ListView_SetItem(hListViewAddress, &stItem);
	}
	cLock.unlock();
}

 void Thread_Update(HWND hDlg)
{
	while (1)
	{
		Sleep(100);
		g_Mem.QueryMemoryValue();
		LVITEMA stItem;
		char szBuffer[20];
		memset(&stItem, 0, sizeof(LVITEMA));
		stItem.mask = LVIF_TEXT | LVCF_SUBITEM;
		stItem.iSubItem = 3;
		int nIndex = 0;
		std::unique_lock<std::mutex> cLock(g_Mem.m_Mutex, std::defer_lock);
		if (cLock.try_lock())
		{
			for (std::list<AddressInfo>::iterator it = g_Mem.m_ListAddress.begin();
				it != g_Mem.m_ListAddress.end(); it++, nIndex++)
			{
				if (it->nCurrentValue != it->nTargetValue)
				{
					stItem.iItem = nIndex;
					wsprintfA(szBuffer, "%ld", it->nCurrentValue);
					stItem.pszText = szBuffer;
					ListView_SetItem(GetDlgItem(hDlg, IDC_LIST_ADDRESS), &stItem);
				}
			}
			cLock.unlock();
		}
	}
}