#include "MemHandle.h"


MemHandle::MemHandle()
{
	m_nPID = 0;
	GetSystemInfo(&m_stSystemInfo);
}

MemHandle::~MemHandle()
{

}

bool MemHandle::ReadMemoryRegion(int nPID)
{
	m_ListMemRegion.clear();
	m_nPID = nPID;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		false, m_nPID);
	if (!hProcess)
		return false;
	MEMORY_BASIC_INFORMATION stMemInfo;
	memset(&stMemInfo, 0, sizeof(MEMORY_BASIC_INFORMATION));
	int nMemSize = sizeof(MEMORY_BASIC_INFORMATION);
	//程序的最高地址和最低地址
	LPVOID pMinAddress = m_stSystemInfo.lpMinimumApplicationAddress;
	LPVOID pMaxAddress = m_stSystemInfo.lpMaximumApplicationAddress;
	//从低地址向高地址遍历内存
	while (pMinAddress < pMaxAddress)
	{
		VirtualQueryEx(hProcess, pMinAddress, &stMemInfo, nMemSize);
		m_ListMemRegion.emplace_back(stMemInfo);
		pMinAddress = (LPVOID)((int)stMemInfo.BaseAddress + (int)stMemInfo.RegionSize);
	}
	CloseHandle(hProcess);
	return true;
}

bool MemHandle::FirstFindMemory(int nValue)
{
	if (!nValue)
		return false;
	if (m_ListMemRegion.empty())
		return false;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		false, m_nPID);
	if (!hProcess)
		return false;
	LPBYTE pBuffer = new byte[m_stSystemInfo.dwPageSize];
	typedef std::list<MEMORY_BASIC_INFORMATION>::iterator  MemItertor;
	std::unique_lock<std::mutex> cLock(m_Mutex,std::defer_lock);
	cLock.lock();
	m_ListAddress.clear();
	for (MemItertor it = m_ListMemRegion.begin();
		it != m_ListMemRegion.end(); it++)
	{
		//该区域没有申请物理内存
		if(it->State != MEM_COMMIT)
			continue;
		int nCurrentPos = 0;
		SIZE_T nReadByte = 0;
		while (nCurrentPos < (int)it->RegionSize)
		{
			LPVOID pAddress = (LPVOID)((int)it->BaseAddress + nCurrentPos);
			ReadProcessMemory(hProcess, pAddress, 
				pBuffer, m_stSystemInfo.dwPageSize, &nReadByte);
			if(nReadByte)
			{
				for (int nIndex = 0;
					nIndex < (int)(m_stSystemInfo.dwPageSize - 3); nIndex++)
				{
					if (*((int*)(pBuffer + nIndex)) == nValue)
					{
						AddressInfo stAddrInfo;
						stAddrInfo.nAddress = (int)((int)pAddress + nIndex);
						stAddrInfo.nTargetValue = stAddrInfo.nCurrentValue = nValue;
						m_ListAddress.emplace_back(stAddrInfo);
					}
				}
			}
			nCurrentPos += m_stSystemInfo.dwPageSize;
		}
	}
	cLock.unlock();
	delete[] pBuffer;
	CloseHandle(hProcess);
	return true;
}

void MemHandle::QueryMemoryValue()
{
	if (m_ListAddress.empty())
		return;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, m_nPID);
	if (!hProcess)
		return;
	std::unique_lock<std::mutex> cLock(m_Mutex,std::defer_lock);
	if (cLock.try_lock())
	{
		for (std::list<AddressInfo>::iterator it = m_ListAddress.begin();
			it != m_ListAddress.end(); it++)
		{
			SIZE_T nReadByte;
			ReadProcessMemory(hProcess, (LPVOID)(it->nAddress),
				&it->nCurrentValue, sizeof(int), &nReadByte);
		}
		cLock.unlock();
	}
	CloseHandle(hProcess);
}

bool MemHandle::NextFindMemory(int nValue)
{
	if (m_ListAddress.empty())
		return false;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		false, m_nPID);
	if (!hProcess)
		return false;
	std::list<AddressInfo> cTempList;
	SIZE_T nReadByte;
	int nCurrentValue;
	typedef std::list<AddressInfo>::iterator AddIterator;
	std::unique_lock<std::mutex> cLock(m_Mutex, std::defer_lock);
	cLock.lock();
	for (AddIterator it = m_ListAddress.begin(); it != m_ListAddress.end(); it++)
	{
		ReadProcessMemory(hProcess, (LPVOID)(it->nAddress), 
			&nCurrentValue,sizeof(int),&nReadByte);
		if(!nReadByte)
			continue;
		if (nCurrentValue == nValue)
		{
			it->nTargetValue = it->nCurrentValue = nValue;
			cTempList.emplace_back(*it);
		}
	}
	m_ListAddress.clear();
	m_ListAddress.insert(m_ListAddress.begin(), cTempList.begin(), cTempList.end());
	cLock.unlock();
	CloseHandle(hProcess);
	return true;
}

bool MemHandle::NodifyMemory(int nAddress, int nValue)
{
	if (!nAddress)
		return false;
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, m_nPID);
	if (!hProcess)
		return false;
	DWORD dwOldMode;
	SIZE_T nWriteByte;
	VirtualProtectEx(hProcess, (LPVOID)nAddress, sizeof(int), PAGE_READWRITE, &dwOldMode);
	WriteProcessMemory(hProcess, (LPVOID)nAddress, &nValue, sizeof(int), &nWriteByte);
	VirtualProtectEx(hProcess, (LPVOID)nAddress, sizeof(int), dwOldMode, 0);
	CloseHandle(hProcess);
	if(!nWriteByte)
		return false;
	return true;
}


