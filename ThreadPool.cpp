#include <iostream>
#include "./Crawler.h"
using namespace std;

/*************************************
函数： ThreadPool
描述： 线程池构造函数
参数：
1、void(*Func)(T)：所要执行的任务函数
2、threadNum：线程数量
3、maxJobQueueLen ：任务队列的最大长度
返回值：
无
*************************************/
ThreadPool::ThreadPool(void(*Func)(string,ThreadPool*),size_t threadNum,size_t maxJobQueueLen):m_userCallee(Func),m_threadNum(threadNum),m_maxJobQueueLen(maxJobQueueLen),m_completeJobNum(0),m_threadpoolIsClosed(false)
{
	//初始化工作任务队列互斥量
	pthread_mutex_init(&m_jobQueueMutex,NULL);
	pthread_mutex_init(&m_jobSetMutex,NULL);

	//初始化条件变量
	pthread_cond_init(&m_jobQueueIsNotEmpty,NULL);
	pthread_cond_init(&m_jobQueueIsNotFull,NULL);
	
	pthread_t tidp;
	
	//使用this指针作为线程执行函数的参数
	ThreadPool* arg=this;

	//循环创建m_threadNum个线程，建立线程池
	for(size_t i=0;i<m_threadNum;i++)
	{
		//每个线程执行threadpool_caller函数，以this为参数
		pthread_create(&tidp,NULL,threadpool_caller,(void *)arg);
		//将该线程加入到线程队列中
		m_threadIdList.push_back(tidp);
	}
}

/******************************************
函数：~ThreadPool()
描述：析构函数
参数：无
返回值：无
******************************************/
ThreadPool::~ThreadPool()
{
	if(m_threadpoolIsClosed == false)
		threadpool_destroy();
}

void ThreadPool::threadpool_destroy()
{
	if(m_threadpoolIsClosed == true)
		return ;
	m_threadpoolIsClosed = true;

	//发出队列未满以及队列未空信号，迫使threadpool_caller函数以及“添加工作函数”停止等待信号
	pthread_cond_broadcast(&m_jobQueueIsNotEmpty);
	pthread_cond_broadcast(&m_jobQueueIsNotFull);

	//遍历线程池，结束线程
	list<pthread_t>::iterator tmp=m_threadIdList.begin();
	while(tmp!=m_threadIdList.end())
	{
		pthread_join(*tmp,NULL);
		tmp++;
	}
	
	//销毁互斥量以及条件变量
	pthread_mutex_destroy(&m_jobQueueMutex);
	pthread_cond_destroy(&m_jobQueueIsNotEmpty);
	pthread_cond_destroy(&m_jobQueueIsNotFull);

}

/*****************************************
函数：threadpool_caller
描述：单个线程的执行函数
参数：
1、arg:以this指针作为参数
返回值：
无
*****************************************/
void* ThreadPool::threadpool_caller(void * arg)
{
	ThreadPool* pTP = (ThreadPool *)arg;

	//循环监听
	while(1)
	{
		//锁住任务队列互斥量
		pthread_mutex_lock(&(pTP->m_jobQueueMutex));

		//循环监听，如果任务队列为空并且线程池未关闭，则阻塞等待条件变量，同时释放互斥锁
		//直到“添加工作”函数的未空信号唤醒该进程
		while((pTP->m_jobToDoQueue.empty()==true)&&(pTP->m_threadpoolIsClosed==false))
		{
			pthread_cond_wait(&(pTP->m_jobQueueIsNotEmpty),&(pTP->m_jobQueueMutex));
		}

		//若线程池关闭
		if(pTP->m_threadpoolIsClosed == true)
		{
			//释放互斥锁，并调用pthread_exit()与析构函数的pthread_join对应
			pthread_mutex_unlock(&(pTP->m_jobQueueMutex));
			pthread_exit(NULL);
		}

		//若线程池正常运行,则从任务队列头部取出一个任务，并将完成工作的数量加1
		string job = pTP->threadpool_get_job();
		++(pTP->m_completeJobNum);

		//若此时取出的正好是工作队列满时的一个任务，则执行完取出操作后，工作队列未满，此时发出未满
		//的条件变量信号，该信号用于唤醒阻塞的“添加工作”函数
		if((pTP->m_jobToDoQueue).size() == (pTP->m_maxJobQueueLen)-1)
		{
			pthread_cond_broadcast(&(pTP->m_jobQueueIsNotFull));
		}

		//解锁任务队列互斥量
		pthread_mutex_unlock(&(pTP->m_jobQueueMutex));

		//执行任务函数
		if((pTP->m_userCallee)!=NULL)
		{
			(*(pTP->m_userCallee))(job,pTP);
		}

	}
}

/*******************************
函数：threadpool_get_job()
功能：从任务队列中取出一个头任务
参数：
无
返回值：
1、工作编号
*******************************/
string ThreadPool::threadpool_get_job()
{
	string job = m_jobToDoQueue.front();
	m_jobToDoQueue.pop();
	return job;
}

/**************************************
函数：threadpool_add_job
描述：向任务队列中添加新任务
参数：
1、job: 要添加的任务
返回值：
1、int:是否调用成功
**************************************/
int ThreadPool::threadpool_add_job(const string& job)
{
	//job队列去冗余功能，用于爬虫的url去重功能
	pthread_mutex_lock(&m_jobSetMutex);

	//如果set中已有该元素，则不添加
	if(m_jobAllSet.count(job))
	{
		pthread_mutex_unlock(&m_jobSetMutex);
		return 0;
	}

	m_jobAllSet.insert(job);
	pthread_mutex_unlock(&m_jobSetMutex);


	//锁住任务队列互斥量
	pthread_mutex_lock(&m_jobQueueMutex);

       //循环监听：如果任务队列已满且线程池未关闭则阻塞线程释放互斥量，直到threadpool_caller函数发出的队列未满信号到达
	while((m_jobToDoQueue.size() == m_maxJobQueueLen)&&(m_threadpoolIsClosed == false))
	{
		pthread_cond_wait(&m_jobQueueIsNotFull,&m_jobQueueMutex);
	}

	//若线程池关闭，则释放互斥量并返回-1
	if(m_threadpoolIsClosed == true)
	{
		pthread_mutex_unlock(&m_jobQueueMutex);
		return 0;
	}

	//若线程池未关闭，则在队列尾部加入新任务
	m_jobToDoQueue.push(job);

	//

	//若此时任务队列刚好有一个任务，则广播未空信号，唤醒所有阻塞的在threadpool_caller的线程
	
	if(m_jobToDoQueue.size() == 1)
	{
		pthread_cond_broadcast(&m_jobQueueIsNotEmpty);
	}

	pthread_mutex_unlock(&m_jobQueueMutex);

	return 1;

}






































