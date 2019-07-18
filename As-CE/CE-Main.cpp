#include "resource.h"
#include "MemHandle.h"

//�����Ӿ�Ч��
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")  

MemHandle g_Mem;

//���ڹ���
INT_PTR CALLBACK TheDialogProc(HWND,UINT,WPARAM,LPARAM);
//��ʼ���¼�
void OnInitiailze(HWND);
//�ؼ���Ϣ�¼�
void OnNotify(HWND, WPARAM, LPARAM);
//�ؼ���Ϣ�¼�
void OnCommand(HWND, WPARAM, LPARAM);

//���½����б�
void UpdateProcess(HWND);
//˫��ָ������
void DBlickProcess(HWND);
//��ȡ�����ڴ�����
void ReadProcessMemRegion(HWND);
//��һ���ڴ����
void FirstMemorySearch(HWND);
//��һ���ڴ����
void NextMemorySearch(HWND);
//���ò���
void ReScanSearch(HWND);
//�޸��ڴ��е���ֵ
void HackMemoryValue(HWND);
//�����ַ��Ϣ
void Thread_Address(HWND);
//��ʱ������������ĵ�ַ����ֵ
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
	//���½����б�
	UpdateProcess(hDlg);
	//��ȡ��ͼ���
	HWND hViewAddress = GetDlgItem(hDlg, IDC_LIST_ADDRESS);
	//����Ϊ����ģʽ
	ListView_SetView(hViewAddress, LVS_REPORT);
	//������չ��ʽ
	ListView_SetExtendedListViewStyle(hViewAddress, LVS_EX_AUTOAUTOARRANGE| LVS_EX_AUTOSIZECOLUMNS| LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER| LVS_EX_GRIDLINES);
	//���Column��Ϣ
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
	//���ð�ť��״̬
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_NEXT), false);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_HACK), false);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_RESCAN), false);
}

void OnNotify(HWND hDlg, WPARAM wParam,LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_LIST_ADDRESS)
	{
		if (((LPNMHDR)lParam)->code == NM_CLICK)//����
		{
			//��ȡ��ַ
			char szBuffer[20];
			ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_ADDRESS),
				((LPNMITEMACTIVATE)lParam)->iItem, 1, szBuffer, 20);
			Edit_SetText(GetDlgItem(hDlg, IDC_EDIT_TIPS), szBuffer);
		}
		else if (((LPNMHDR)lParam)->code == NM_DBLCLK)//˫��
		{
			//������ʵ�����б���ͼ�У�˫�����ܾ͵ر༭note�ڴ���ֵ��˵���ı�������˼·����˫����ʱ����ָ��note���洴��һ��Edit�ؼ����༭��ɺ��ȡ�ı���ɾ��Edit�ؼ������ı��ŵ�ָ����note���档
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
	//���ﴦ��˫�������б��ָ�����̵�
	if (LOWORD(wParam) == IDC_LIST_PROCESS && HIWORD(wParam) == LBN_DBLCLK)
	{
		DBlickProcess(hDlg);
		return;
	}
	//��ť�ж�
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
	//��ȡ�����б���
	HWND hListProcess = GetDlgItem(hDlg, IDC_LIST_PROCESS);
	//��ս����б�
	ListBox_ResetContent(hListProcess);
	//�������̱���
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!hSnap)
		return;
	char szFormat[200];
	PROCESSENTRY32 stProcInfo;
	memset(&stProcInfo, 0, sizeof(PROCESSENTRY32));
	stProcInfo.dwSize = sizeof(PROCESSENTRY32);
	//���ҵ�һ��������Ϣ
	bool bRet = Process32First(hSnap, &stProcInfo);
	while (bRet)
	{
		//��ʽ���ַ���
		wsprintfA(szFormat, "%d     [%s]", 
			stProcInfo.th32ProcessID, stProcInfo.szExeFile);
		//��ӵ������б�
		ListBox_AddString(hListProcess, szFormat);
		//������һ��������Ϣ
		bRet = Process32Next(hSnap, &stProcInfo);
	}
	CloseHandle(hSnap);
}

void DBlickProcess(HWND hDlg)
{
	//��ȡ�����б���
	HWND hListProcess = GetDlgItem(hDlg, IDC_LIST_PROCESS);
	//��ȡ�����б��˫�������
	int nProcessIndex = ListBox_GetCurSel(hListProcess);
	char szBuffer[200];
	//��ȡ������Ϣ
	ListBox_GetText(hListProcess, nProcessIndex, szBuffer);
	//��ȡĿ����̱༭����
	HWND hEditTarget = GetDlgItem(hDlg, IDC_EDIT_TARGETPROCESS);
	//����Ŀ����̱༭��
	Edit_SetText(hEditTarget, szBuffer);
}

void ReadProcessMemRegion(HWND hDlg)
{
	//��ȡĿ����̱༭����
	HWND hEditProcess = GetDlgItem(hDlg, IDC_EDIT_TARGETPROCESS);
	//��ȡָ������
	char szProcess[200];
	Edit_GetText(hEditProcess, szProcess, 200);
	//��ȡ����ID
	int nPID = 0;
	sscanf(szProcess, "%d", &nPID);
	//��ȡ��ʾ���ľ��
	HWND hEditTips = GetDlgItem(hDlg, IDC_EDIT_TIPS);
	//��ȡ�����ڴ�����
	if (g_Mem.ReadMemoryRegion(nPID))
		Edit_SetText(hEditTips, "[Tips]: Read memory regioin success");
	else
		Edit_SetText(hEditTips, "[Tips]: Read memory regioin fail");
}

void FirstMemorySearch(HWND hDlg)
{
	//��ȡ������ֵ
	int nValue = GetDlgItemInt(hDlg, IDC_EDIT_VALUE, 0, 0);
	//��ȡ��ʾ���ľ��
	HWND hEditTips = GetDlgItem(hDlg, IDC_EDIT_TIPS);
	//�����ڴ���ֵ
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
	//����һ���߳���ʾ��ַ��Ϣ
	std::thread cAdd(Thread_Address, GetDlgItem(hDlg, IDC_LIST_ADDRESS));
	cAdd.detach();
}

void NextMemorySearch(HWND hDlg)
{
	//��ȡ������ֵ
	int nValue = GetDlgItemInt(hDlg, IDC_EDIT_VALUE, 0, 0);
	//��ȡ��ʾ���ľ��
	HWND hEditTips = GetDlgItem(hDlg, IDC_EDIT_TIPS);
	if (g_Mem.NextFindMemory(nValue))
		Edit_SetText(hEditTips, "[Tips]: Next search memory value success");
	else
		Edit_SetText(hEditTips, "[Tips]: Next search memory value success");
	//����һ���߳���ʾ��ַ��Ϣ
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
	//��ȡ��ֵ
	int nValue = GetDlgItemInt(hDlg, IDC_EDIT_NODIFYVALUE, 0, 0);
	//��ȡ��ʾ��ľ��
	HWND hEditTips = GetDlgItem(hDlg, IDC_EDIT_TIPS);
	//��ȡ��ַ
	char szAddress[30];
	Edit_GetText(hEditTips, szAddress, 30);
	//����ַ����ת��
	int nAddress;
	sscanf(&szAddress[2], "%x", &nAddress);
	//�޸��ڴ���ֵ
	if (g_Mem.NodifyMemory(nAddress, nValue))
		Edit_SetText(hEditTips, "[Tips]: Hack success");
	else
		Edit_SetText(hEditTips, "[Tips]: Hack fail");
}

void Thread_Address(HWND hListViewAddress)
{
	//��ԭ��������ȫ�����
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