#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include <TlHelp32.h>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

typedef struct _AddressInfo
{
	int nAddress;
	int nTargetValue;
	int nCurrentValue;
	char szNote[50];
	_AddressInfo()
	{
		nAddress = 0;
		nTargetValue = 0;
		nCurrentValue = 0;
		szNote[0] = '\0';
	}
}AddressInfo,*PAddressInfo;

class MemHandle
{
public:
	//进程的ID
	int m_nPID;
	//系统信息
	SYSTEM_INFO m_stSystemInfo;
	//进程的内存区域列表
	std::list<MEMORY_BASIC_INFORMATION> m_ListMemRegion;
	//地址列表
	std::list<AddressInfo> m_ListAddress;
	//线程同步锁
	std::mutex m_Mutex;
public:
	MemHandle();
	~MemHandle();

public:
	//读取指定进程的内存区域
	bool ReadMemoryRegion(int nPID);

	//第一次内存查找
	bool FirstFindMemory(int nValue);

	//查找容器中当前地址的内存数值
	void QueryMemoryValue();

	//下一次内存查找
	bool NextFindMemory(int nValue);

	//修改指定内存
	bool NodifyMemory(int nAddress, int nValue);
};

