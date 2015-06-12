# myengine
    安装方法：
    1、进入myengine文件夹；
    2、运行make命令，运行成功后会在本文件夹中生成名为IndexEngine的可执行文件；
    3、运行./IndexEngine即可启动程序
    
    程序包含模块：
    
    1、线程池模块：
    功能简介：本模块利用Linux环境中的线程机制与条件变量机制构建了可以同时执行多个任务的线程池，在多个线程访问Url队列时，提供互斥锁机制。线程函数执行添加URL到队列的功能。
    编程注意事项：在C++的类中，普通成员函数不能作为pthread_create()函数的线程函数，必须声明为static类型，因为线程函数要求必须是全局函数，而static函数可看成是全局函数，它无this指针，但此时static函数只能访问类中的static成员，而不能访问非static成员，可将this指针作为线程函数的参数传给该线程。 
    
    
