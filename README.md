# myengine
    安装方法：
    1、进入myengine文件夹；
    2、运行make命令，运行成功后会在本文件夹中生成名为IndexEngine的可执行文件；
    3、运行./IndexEngine即可启动程序
    
    程序包含模块：
    
    1、线程池模块：
    功能简介：本模块利用Linux环境中的线程机制与条件变量机制构建了可以同时执行多个任务的线程池，在多个线程访问Url队列时，提供互斥锁机制。线程函数执行添加URL到队列的功能。
    编程注意事项：在C++的类中，普通成员函数不能作为pthread_create()函数的线程函数，必须声明为static类型，因为线程函数要求必须是全局函数，而static函数可看成是全局函数，它无this指针，但此时static函数只能访问类中的static成员，而不能访问非static成员，可将this指针作为线程函数的参数传给该线程。 
    
    2、关键词搜索模块：
    功能简介：通过识别用户输入的关键词，能够利用Boost库中的tokenizer库对其进行分词。关键词匹配分为两种：若用户输入的关键词在某网页中完全匹配，则保存该网页；若不能完全匹配，则采用tf-idf算法对关键词的权重进行计算，找出该句子中最重要的关键词，并对其进行匹配。
    
    3、网页爬取模块：
    功能简介：该模块分为两个单元。首先对Url进行分析，包括获取Url的协议名称、域名以及资源路径，并通过Boost库中Asio库向Http发出请求，获取网页内容。第二个单元为网页链接的获取单元，其原理为对网页中的HTML语言进行分析，找到<a>标签，并提取href=后的链接地址，分不同链接种类对Url进行提取（内部链接与外部链接）
    
    4、网页存储单元
    功能简介：将爬取下来的网页存入本地文件夹，并形成网页URL搜索目录文件以便查询。
