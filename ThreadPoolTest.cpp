#include "./ThreadPool.h"

void  user_callee(string data,ThreadPool * pool)
{
	pthread_t tid = pthread_self();

	cout<<"My thread id is "<<tid<<endl;
	cout<<"My job is to display "<<data<<endl;
	pool->threadpool_add_job(data+'1');
	sleep(3);
}

int main()
{
	ThreadPool testPool(user_callee,10,200);
	testPool.threadpool_add_job("1");
	testPool.threadpool_add_job("2");
	testPool.threadpool_add_job("3");
	testPool.threadpool_add_job("4");
	testPool.threadpool_add_job("5");
	testPool.threadpool_add_job("6");

	while(1);

	return 0;
}
