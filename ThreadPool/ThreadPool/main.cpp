// ThreadPool.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <iostream>
#include <crtdbg.h>
#include "ThreadPoolBase.h"

inline void EnableMemLeakCheck()
{
    //_CrtSetBreakAlloc(195); 
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
}

#ifdef _DEBUG
#define  new   new(_NORMAL_BLOCK, __FILE__, __LINE__) 
#endif


UINT __stdcall TestFun1(VOID* pArguments);
UINT __stdcall TestFun2(VOID* pArguments);
UINT __stdcall TestFun3(VOID* pArguments);
UINT __stdcall TestFun4(VOID* pArguments);

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef _DEBUG
    EnableMemLeakCheck();
#endif

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    CThreadPoolBase<2> one;
    one.CreateTasks(TestFun1, NULL);
    one.CreateTasks(TestFun2, NULL);

    ::MessageBox(NULL, _T("�ȴ�2 ִ�����"), _T(""), 0);
    one.CreateTasks(TestFun3, NULL);

    ::MessageBox(NULL, _T("�����߳�4"), _T(""), 0);
    cout << "��ʼ�����߳�4 �ȴ��߳�3�˳�" << endl;

    one.CreateTasks(TestFun4, NULL);

    ::MessageBox(NULL, _T("wait"), _T(""), 0);
    one.StopThreadPool(TRUE);

	return 0;
}

UINT __stdcall TestFun1(VOID* pArguments)
{
    UNREFERENCED_PARAMETER(pArguments);

    while (true)
    {
        cout << "TestFun1 -------------------- " << endl;
        Sleep(1000);
    }

    return 0;
}

UINT __stdcall TestFun2(VOID* pArguments)
{
    UNREFERENCED_PARAMETER(pArguments);
    int i = 0;
    while (true)
    {
        cout << "TestFun2 ********************" << endl;
        Sleep(1000);
        i++;

        if (i == 5)
        {
            break;
        }
    }

    cout << "�߳�2ִ�����" << endl;
    return 0;
}

UINT __stdcall TestFun3(VOID* pArguments)
{
    UNREFERENCED_PARAMETER(pArguments);
    int i = 0;
    while (true)
    {
        cout << "TestFun3" << endl;
        Sleep(1000);
        i++;

        if (i == 10)
        {
            break;
        }
    }

    cout << "�߳�3 �˳�" << endl;
    return 0;
}

UINT __stdcall TestFun4(VOID* pArguments)
{
    UNREFERENCED_PARAMETER(pArguments);
    while (true)
    {
        cout << "TestFun4 ********************" << endl;
        Sleep(1000);
    }

    return 0;
}
