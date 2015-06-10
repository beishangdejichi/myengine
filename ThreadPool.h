#ifndef _THREADPOOL_H
#define _THREAD_H

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <list>
#include <set>
#include <string>
#include <pthread.h>

using namespace std;

class ThreadPool
{
	public:
		ThreadPool(void(*Func)(string,ThreadPool *) = NULL,size_t threadNum = 10,size_t maxJobQueueLen = 50);
		~ThreadPool();
		void threadpool_destroy();
		int threadpool_add_job(const string& job);
	private:
		static void* threadpool_caller(void * arg);
		string threadpool_get_job();

		//存储任务函数指针
		void (*m_userCallee)(string,ThreadPool*);
		
		//线程池中的线程数量
		const size_t m_threadNum;

		//工作队列的最大长度
		const size_t m_maxJobQueueLen;

		//已完成的工作数
		size_t m_completeJobNum;

		//判空和判满的条件变量
		pthread_cond_t m_jobQueueIsNotEmpty;
		pthread_cond_t m_jobQueueIsNotFull;

		//工作队列的互斥量
		pthread_mutex_t m_jobQueueMutex;

		//无重复的工作队列的互斥量
		pthread_mutex_t m_jobSetMutex;

		//工作任务队列
		queue<string> m_jobToDoQueue;

		//无重复的工作任务队列
		set<string> m_jobAllSet;

		//线程池链表
		list<pthread_t> m_threadIdList;
		
		//线程池关闭标志位
		bool m_threadpoolIsClosed;
};
//#include "ThreadPool.cpp"
#endif
