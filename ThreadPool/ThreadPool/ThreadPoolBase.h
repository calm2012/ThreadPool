// ------------------------------------------------------------------------------------------------------------------------
// FileName:  ThreadPoolBase.h
//
// author  :  Calm2012
//
// purpose :  线程池
// 
// Remark  :  
//
// @date      2017/03/31
//
// ------------------------------------------------------------------------------------------------------------------------

#pragma once
#include <WTypes.h>
#include <vector>
#include <process.h>
#include <assert.h>


enum 
{
    THREAD_MSG_INSERT_NEW_THREAD = WM_USER+1,
    THREAD_MSG_RUN_THREAD,
};

typedef  UINT (__stdcall* ThreadFun_Type)(VOID* pArguments);

const int MAX_THREADS_POOL = 100;

template< int nMaxThreas = MAX_THREADS_POOL, BOOL bFixedSize = TRUE>
class CThreadPoolBase
{
public:

    typedef struct _THREAD_DATA
    {
         DWORD dwTaskID;
         DWORD  dwThreadID;
         HANDLE hThread;
         HANDLE hEvent;
         HANDLE hMsgEvent;

         ThreadFun_Type pFun;
         VOID* pParam;

         CThreadPoolBase* pCThreadPoolBase;

         _THREAD_DATA() : dwTaskID(0), dwThreadID(0), hThread(NULL), hEvent(NULL), pFun(NULL), pParam(NULL), hMsgEvent(NULL), pCThreadPoolBase(NULL)
         {
         }
    }THREAD_DATA, *PTHREAD_DATA;

public:
    BOOL CreateTasks(ThreadFun_Type pFun, VOID* pParam);
    VOID StopThreadPool(BOOL bAsyn = FALSE);
    int GetIdleThreadCount();

private:
    static UINT __stdcall EntryPoint(VOID* pParam);

private:
    BOOL InsertTasks(ThreadFun_Type pFun, VOID* pParam);
    BOOL CreateTasksThread(THREAD_DATA& data);
    BOOL IsThreadIdle(HANDLE hEvnet);
    UINT Run();

public:
    CThreadPoolBase(void);
    virtual ~CThreadPoolBase(void);

private:
    CThreadPoolBase(const CThreadPoolBase&);
    CThreadPoolBase& operator = (const CThreadPoolBase&);

private:
    std::vector<HANDLE>         m_vecThreadHandle;
    std::vector<HANDLE>         m_vecThreadEvent;
    std::vector<THREAD_DATA>    m_vecThread;
    THREAD_DATA                 m_ThreadMgr;
};

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::CThreadPoolBase(void)
{
    m_vecThreadHandle.reserve(nMaxThreas);
    m_vecThreadEvent.reserve(nMaxThreas);
    m_vecThread.reserve(nMaxThreas);

    m_ThreadMgr.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    m_ThreadMgr.hMsgEvent = ::CreateEvent( NULL, TRUE, FALSE, NULL );
    m_ThreadMgr.pCThreadPoolBase = this;
    m_ThreadMgr.hThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, EntryPoint, &m_ThreadMgr, 0, reinterpret_cast<UINT*>(&m_ThreadMgr.dwThreadID)));
}

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::~CThreadPoolBase(void)
{
    if(m_ThreadMgr.hEvent != NULL)
    {
        ::CloseHandle(m_ThreadMgr.hEvent);
        m_ThreadMgr.hEvent = NULL;
    }
    if (m_ThreadMgr.hMsgEvent != NULL)
    {
        ::CloseHandle(m_ThreadMgr.hMsgEvent);
        m_ThreadMgr.hMsgEvent = NULL;
    }

    if(m_ThreadMgr.hThread != NULL)
    {
        ::TerminateThread(m_ThreadMgr.hThread, DWORD(-1));
        m_ThreadMgr.hThread = NULL;
    }
}

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
BOOL CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::CreateTasks(ThreadFun_Type pFun, VOID* pParam)
{
    if( m_ThreadMgr.hMsgEvent != NULL )
    {
        WaitForSingleObject(m_ThreadMgr.hMsgEvent, INFINITE);
        ::CloseHandle(m_ThreadMgr.hMsgEvent);
        m_ThreadMgr.hMsgEvent = NULL;
    }

    return ::PostThreadMessage(m_ThreadMgr.dwThreadID, THREAD_MSG_INSERT_NEW_THREAD, reinterpret_cast<WPARAM>(pFun), reinterpret_cast<LPARAM>(pParam));
}

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
UINT __stdcall CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::EntryPoint(VOID* pParam)
{
    THREAD_DATA* pData = reinterpret_cast<THREAD_DATA*>(pParam);
    MSG msg;
    ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ); //让线程创建消息队列
    ::SetEvent(pData->hMsgEvent);
    return pData->pCThreadPoolBase->Run();
}

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
UINT CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::Run()
{
    MSG msg;
    while(::GetMessage(&msg, NULL, 0, 0) )
    {
        switch( msg.message )
        {
        case THREAD_MSG_INSERT_NEW_THREAD:
            {
                InsertTasks(reinterpret_cast<ThreadFun_Type>(msg.wParam), reinterpret_cast<VOID*>(msg.lParam));
            }
            break;
        case THREAD_MSG_RUN_THREAD:
            {
                THREAD_DATA* pData = reinterpret_cast<THREAD_DATA*>(msg.lParam);
                if (pData)
                {
                    ThreadFun_Type pFun = reinterpret_cast<ThreadFun_Type>(pData->pFun);
                    if (pFun)
                    {
                        pFun(reinterpret_cast<VOID*>(pData->pParam));
                        ::SetEvent(pData->hEvent);
                    }
                }
            }
            break;

        default:
            DispatchMessage(&msg);
            break;
        } 
    }

    return 0;
}

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
BOOL CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::InsertTasks(ThreadFun_Type pFun, VOID* pParam)
{
    THREAD_DATA data;
    UINT nIndex = 0;
    BOOL bHaveIdleThread = FALSE;
    for (UINT i = 0; i < m_vecThreadEvent.size(); i++, nIndex++)
    {
        if (IsThreadIdle(m_vecThreadEvent[i]))
        {
            bHaveIdleThread = TRUE;
            nIndex = i;
            break;
        }
    }

    if (bHaveIdleThread)                            // 存在空闲线程
    {
        m_vecThread[nIndex].pFun = pFun;
        m_vecThread[nIndex].pParam = pParam;
        m_vecThread[nIndex].dwTaskID = nIndex;

        ::ResetEvent(m_vecThread[nIndex].hEvent);
        return ::PostThreadMessage(m_vecThread[nIndex].dwThreadID, THREAD_MSG_RUN_THREAD, NULL, reinterpret_cast<LPARAM>(&m_vecThread[nIndex]));
    }

    // 没有空闲线程
    if (m_vecThreadHandle.size() >= nMaxThreas)     // 线程池已满
    {
        if (bFixedSize)                             // 非自增线程池
        {
            DWORD dwObj = ::WaitForMultipleObjects(m_vecThreadEvent.size(), &m_vecThreadEvent[0], FALSE, INFINITE);
            m_vecThread[dwObj].pFun = pFun;
            m_vecThread[dwObj].pParam = pParam;
            m_vecThread[dwObj].dwTaskID = dwObj;

            ::ResetEvent(m_vecThread[dwObj].hEvent);
            return ::PostThreadMessage(m_vecThread[dwObj].dwThreadID, THREAD_MSG_RUN_THREAD, NULL, reinterpret_cast<LPARAM>(&m_vecThread[dwObj]));
        }
    }

    if (!CreateTasksThread(data))
    {
        assert(0);
        return FALSE;
    }

    if (0 != data.dwThreadID && data.hThread && data.hEvent)
    {
        m_vecThreadHandle.push_back(data.hThread);
        m_vecThreadEvent.push_back(data.hEvent);

        data.pCThreadPoolBase = this;
        data.pFun = pFun;
        data.pParam = pParam;
        data.dwTaskID = nIndex;
        m_vecThread.push_back(data);

        nIndex = m_vecThread.size() - 1;
    }
    else
    {
        assert(0);
        return FALSE;
    }

    if(data.hMsgEvent)
    {
        WaitForSingleObject(data.hMsgEvent, INFINITE);
        ::CloseHandle(m_ThreadMgr.hMsgEvent);
        data.hMsgEvent = NULL;
    }

    return ::PostThreadMessage(m_vecThread[nIndex].dwThreadID, THREAD_MSG_RUN_THREAD, 0, reinterpret_cast<LPARAM>(&m_vecThread[nIndex]));
}

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
BOOL CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::CreateTasksThread(THREAD_DATA& data)
{
    data.hMsgEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!data.hMsgEvent)
        return FALSE;

    data.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!data.hEvent)
        return FALSE;

    data.hThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, EntryPoint, &data, 0, reinterpret_cast<UINT*>(&data.dwThreadID)));
    if (data.hThread <= 0)
        return FALSE;

    return TRUE;
}

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
BOOL CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::IsThreadIdle(HANDLE hEvnet)
{
    DWORD dwRet = ::WaitForSingleObject(hEvnet, 0);
    if(WAIT_OBJECT_0 == dwRet || WAIT_ABANDONED == dwRet || WAIT_FAILED == dwRet )
        return true;
    else
        return false;
}

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
VOID CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::StopThreadPool(BOOL bAsyn = FALSE)
{
    if (!bAsyn)
    {
        UINT uSize = m_vecThreadEvent.size();
        if (uSize > 0)
        {
            ::WaitForMultipleObjects(uSize, &m_vecThreadEvent[0], TRUE, INFINITE);
        }
    }

    if(m_ThreadMgr.hThread != NULL)
    {
        ::TerminateThread(m_ThreadMgr.hThread, DWORD(-1));
        m_ThreadMgr.hThread = NULL;
    }

    for (UINT i = 0; i < m_vecThreadHandle.size(); i++)
    {
        ::TerminateThread(m_vecThreadHandle[i], DWORD(-1));
        ::CloseHandle(m_vecThreadHandle[i]);
        m_vecThreadHandle[i] = NULL;

        ::CloseHandle(m_vecThreadEvent[i]);
        m_vecThreadEvent[i] = NULL;
    }
}

template< int nMaxThreas /*= MAX_THREADS_POOL*/, BOOL bFixedSize /*= TRUE*/ >
int CThreadPoolBase< nMaxThreas /*= MAX_THREADS_POOL*/, bFixedSize /*= TRUE*/ >::GetIdleThreadCount()
{
    int nCount = 0;
    for (UINT i = 0; i < m_vecThreadEvent.size(); i++)
    {
        if (IsThreadIdle(m_vecThreadEvent[i]))
        {
            nCount++;
        }
    }

    if (m_vecThreadEvent.size() < nMaxThreas)
    {
        nCount += nMaxThreas - nCount;
    }

    return nCount;
}
