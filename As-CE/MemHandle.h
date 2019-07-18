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
	//���̵�ID
	int m_nPID;
	//ϵͳ��Ϣ
	SYSTEM_INFO m_stSystemInfo;
	//���̵��ڴ������б�
	std::list<MEMORY_BASIC_INFORMATION> m_ListMemRegion;
	//��ַ�б�
	std::list<AddressInfo> m_ListAddress;
	//�߳�ͬ����
	std::mutex m_Mutex;
public:
	MemHandle();
	~MemHandle();

public:
	//��ȡָ�����̵��ڴ�����
	bool ReadMemoryRegion(int nPID);

	//��һ���ڴ����
	bool FirstFindMemory(int nValue);

	//���������е�ǰ��ַ���ڴ���ֵ
	void QueryMemoryValue();

	//��һ���ڴ����
	bool NextFindMemory(int nValue);

	//�޸�ָ���ڴ�
	bool NodifyMemory(int nAddress, int nValue);
};

