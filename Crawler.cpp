#include "./Crawler.h"
using namespace std;

//static成员变量只能在类内定义，在类外初始化，因为它不是通过对象的构造函数获得的
bool Crawler::m_crawlerIsExited = false;
pthread_mutex_t Crawler::m_saveUrlLogFileMutex = PTHREAD_MUTEX_INITIALIZER;
string Crawler::m_destDirectory;
/****************************************
函数：Crawler
描述：构造函数
参数：
1、Func：每个线程执行的工作函数
2、threadNum：线程池中线程的数量
3、maxUrlQueueLen：工作任务队列的最大长度
返回值：
无
*****************************************/
Crawler::Crawler(void(*Func)(string,ThreadPool* pool),size_t threadNum,size_t maxUrlQueueLen):m_crawlerThreadPool(Func,threadNum , maxUrlQueueLen)
{
	//初始化添加记录互斥量
	//pthread_mutex_init(&m_saveUrlLogFileMutex,NULL);

	//用当前的时间作为本次爬取的目录名称
	get_time_string(m_destDirectory);


}

/*****************************************
函数：~Crawler
描述：析构函数
参数：
无
返回值：
无
*****************************************/
Crawler::~Crawler()
{
	//当本次爬虫关闭时，调用析构函数
	if(m_crawlerIsExited == false)
		crawler_exit();
}

int Crawler::crawler_exit()
{
	if(m_crawlerIsExited == true)
	{
		return -1;
	}

	//关闭爬虫
	m_crawlerIsExited = true;

	//销毁线程池
	m_crawlerThreadPool.threadpool_destroy();

	//销毁存储文件的互斥量
	pthread_mutex_destroy(&m_saveUrlLogFileMutex);

	return 0;

}

/**********************************
函数：crawler_crawl_page
描述：访问与爬取一个网页的内容
参数：
1、url:网址
2、pageContent:存储网页内容的字符串
返回值：
1、int：该网页内容的大小
***********************************/
int Crawler::crawler_crawl_page(const string& url, string& pageContent)
{
	//定义协议、服务器域名、资源的绝对路径
	string protocol,domain,path;
	
	if(parse_url(url,protocol,domain,path)!=0)
	{
		cout<<"url解析错误"<<endl;
		return -1;
	}

	//定义io_service对象，任何asio程序都要定义此对象，用于程序与操作系统的交互
	boost::asio::io_service io_service;

	//error_code是system库的核心类，用于表示错误代码。
	boost::system::error_code ec;

	//Resolver是Asio的域名解析系统，它通过query函数将指定的域名和服务转化为相应的ip和port的endpoint端点
	boost::asio::ip::tcp::resolver resolver(io_service);

	//query指定主机与服务
	boost::asio::ip::tcp::resolver::query query(domain.c_str(),protocol.c_str());

	//resolve函数返回具有该域名和该协议的endpoint（endpoint中指定主机ip地址和端口）列表的头迭代器（具有相同域名的ip地址可能有多个，这里只取一个）
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query,ec);
	
	//如果ec不为0,说明有错误，则捕获该错误并返回
	if(ec)
	{
		cout<<boost::system::system_error(ec).what()<<endl;
		return -1;
	}

	//创建一个socket对象
	boost::asio::ip::tcp::socket socket(io_service);

	//与服务器通信端点连接
	boost::asio::connect(socket,endpoint_iterator,ec);

	//如果ec不为0,说明有错误，则捕获该错误并返回
	if(ec)
	{
		cout<<boost::system::system_error(ec).what()<<endl;
		return -1;
	}

	//cout<<"Crawl Start : "<<url<<endl;


	//建立一个流类型的streambuf缓冲区，用于存储和发送信息。它可通过stl的stream相关函数实现缓冲区操作
	boost::asio::streambuf request;

	//将该流缓冲区与输出流绑定，对ostream的输出操作最终都流入request缓冲区
	std::ostream request_stream(&request);
	
	//发送HTTP请求头部
	request_stream<<"GET "<<path<<" HTTP/1.0\r\n";
	request_stream<<"Accept: */*\r\n";
	request_stream<<"Accept-Language: zh-CN\r\n";
	request_stream<<"User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:33.0) Gecko/20100101 Firefox/33.0\r\n";
	request_stream<<"Host: "<<domain<<" \r\n";
	request_stream<<"Connection: close\r\n\r\n";

	boost::asio::write(socket,request);
	
	//接收服务器响应信息：

	//设置缓冲区
	boost::asio::streambuf response;

	//接收报头状态行信息
	boost::asio::read_until(socket,response,"\r\n",ec);
	if(ec)
	{
		cout<<"状态行信息错误："<<boost::system::system_error(ec).what()<<endl;
		return -1;
	}


	//将流缓冲区与istream相关联，可方便的从流缓冲区中提取信息
	std::istream response_stream(&response);

	//获得版本信息
	string http_version;
	response_stream>>http_version;

	//获得状态码
	size_t status_code;
	response_stream>>status_code;

	// 获得状态信息
	string status_message;
	getline(response_stream,status_message);
	//cout<<http_version<<endl;
	if(!response_stream || http_version.substr(0,5) != "HTTP/")
	{
		cout<<"服务器端协议错误"<<endl;
		return -1;
	}

	if(status_code != 200)
	{
		cout<<"状态码错误: "<<status_code<<"  "<<url<<endl;
		return -1;
	}

	//cout<<"The response information is "<<http_version<<" "<<status_code<<" "<<status_message<<endl;
	

	//开始接收响应消息除状态行之外的包头信息
	boost::asio::read_until(socket,response,"\r\n\r\n",ec);
	if(ec)
	{
		cout<<"包头信息接收错误"<<endl;
		return -1;
	}

	string header;
	//getline从输入行的下一行读取，遇到换行符返回，且不保存换行符
	while(getline(response_stream,header)&&header!="\r")
	{
		//cout<<header<<endl;
	}

	//获取网页正文

	//定义一个字符流用于接收信息
	stringstream pageContentStream;

	//读取网页内容直到遇到EOF

	//transfer_at_least 函数返回transfer_at_least_t类型,该函数将接收到的数据字节数与其参数相比较
	//如果大于等于指定数量则返回,这里指定为1表示接收到数据就返回处理
	while(boost::asio::read(socket,response,boost::asio::transfer_at_least(1),ec))
	{
		//将内容存入字节流中
		pageContentStream<<&response;
	}

	pageContent = pageContentStream.str();
	//返回获取的字节数量
	size_t pageSize = pageContent.size();
	
//	cout<<"00000000000000000000"<<endl;
//	
//	int a;
//	cin>>a;

	//cout<<"The "<<url<<" page crawl success!"<<endl;
	
	return pageSize;
}

/*************************************
 函数：get_time_string
 描述：获取用于创建文件夹的当前时间信息
       例如：Sat_Aug_24_15_48_12_2013
 参数：
 1、timeStr:存储时间信息
 返回值：
 无
*************************************/
void Crawler::get_time_string(string& timeStr)
{
	time_t t;
	time(&t);

	//返回的timeStr字符串中的末尾带有'\0'
	timeStr=string(ctime(&t)); 
	//将字符串中的空格或冒号变为下划线，并去掉末尾的换行符和'\0'
	for(string::iterator iter = timeStr.begin(); iter != timeStr.end() ; ++ iter)
	{
		if((*iter == ' ') || (*iter == ':'))
			*iter = '_';
		else if(*iter == '\n')
		{
			iter = timeStr.erase(iter);   //删除元素后string的大小自动调整
			break;
		}
	}
}

/****************************************
函数：parse_url
描述：通过url解析出协议、域名和资源路径
参数：
1、url：网址
2、protocol：存储解析出的协议
3、domain: 存储解析出的域名
4、path：资源路径
返回值：
1、调用是否成功
*****************************************/
int Crawler::parse_url(const string& url,string& protocol,string& domain,string& path)
{
	size_t posFound,posStart;

	//获取协议类型
	posFound = url.find("://");

	if(posFound == string::npos || posFound == 0)
	{
		cout<<"协议解析错误"<<endl;
		return -1;
	}

	protocol = url.substr(0,posFound);

	//获取服务器域名地址

	posStart = posFound+3;

	if((protocol.size()+3) >= url.size())
	{
		cout<<"服务器域名解析错误"<<endl;
		return -1;
	}

	posFound = url.find('/',posStart);

	//若出现http:///的形式则返回
	if(posFound == posStart)
	{
		cout<<"服务器域名解析错误"<<endl;
		return -1;
	}
	//若地址中只包含域名，则直接取其后的所有字符串为域名地址
	else if(posFound == string::npos)
	{
		domain = url.substr(posStart,url.size()-posStart);
	}
	else 
	{
		domain = url.substr(posStart,posFound - posStart);
	}

	//获取资源路径
	
	posStart = posFound;
	
	//若无资源路径，则默认使用根路径“/”
	if( (protocol.size()+3+domain.size()) >= url.size())
	{
		path = string("/");
	}
	else
	{
		size_t pathlen = protocol.size()+3+domain.size(); //此处正是“/”的位置
		path = url.substr(pathlen,url.size()-pathlen);
	}

//	cout<<"the url is: "<<url<<" the protocol is: "<<protocol<<" the domain is: "<<domain<<" the path is "<<path<<endl;

	return 0;
}

/*********************************
函数：convert_url_to_file_name
描述：将url转换为文件名可用的字符串
参数：
1、url：网址
2、fileName：存储转换后的文件名
返回值：
无
*********************************/
 void Crawler::convert_url_to_file_name(const string& url,string& fileName)
{
	fileName = url;

	for(size_t index = 0; index < fileName.size(); ++index)
	{
		//文件名中不能包含代表根目录的“/”，也尽量避免冒号
		if((fileName[index] == '/')||(fileName[index] == ':'))
			fileName[index] = '-';
		//文件名中应避免使用转义字符和空格
		else if((fileName[index]=='?')||(fileName[index] == '*')||(fileName[index] == ' ')||(fileName[index] == '&'))
			fileName[index] = '_';
	}

}

/*********************************
函数：crawler_main_loop
描述：每个线程工作的主流程函数
参数：
1、url：网址
2、pool：线程池
返回值：
无
*********************************/
void Crawler::crawler_main_loop(string url,ThreadPool *pool)
{
	string pageContent;         //存储每个网页的内容

	//爬取网页内容
	if(crawler_crawl_page(url,pageContent) < 0)
	{
		cout<<"爬去网页失败"<<endl;
		return;
	}

	//保存网页至当前目录中
	if(crawler_save_page(m_destDirectory,url,pageContent) < 0)
	{
		cout<<"保存网页失败"<<endl;
		return;
	}

	list<string> externalUrlList,internalUrlList; //存储其他网站网址以及站内链接

	//获取该网页中的所有网址与链接
	if(crawler_extract_url(url,pageContent,externalUrlList,internalUrlList)<0)
	{
		cout<<"获取网页中的网址错误"<<endl;
		return;
	}

	//将获得的站内链接放入工作线程池中
	if(crawler_add_url(pool,internalUrlList)<0)
	{
		cout<<"将url放入线程池失败"<<endl;
		return;
	}
}

/**********************************
函数：crawler_save_page
描述：将爬取的网页存储成文本形式
参数：
1、destDirectory：目录名
2、url：网址
3、pageContent：网页内容、
返回值：
1、int：调用成功返回0 失败返回-1
**********************************/
int Crawler::crawler_save_page(const string& destDirectory,const string& url,const string& pageContent)
{
	//定义一个静态变量，标记目录是否被创建
	static bool isDestDirectoryCreated = false;

	if(!isDestDirectoryCreated)
	{
		//创建目录，设置用户权限，如果已经存在该目录，则mkdir置为EEXIST
		if(mkdir(destDirectory.c_str(),S_IRWXU|S_IRWXG|S_IRWXO)<0)
		{
			if(errno != EEXIST)
			{
				cout<<"目录创建失败\n";
				return -1;
			}
		}
		else
		{
			isDestDirectoryCreated = true;
		}
	}
	
	//将url转化为合法的文件名
	string pageContentFileName;
	convert_url_to_file_name(url,pageContentFileName);

	//网页文件的路径 
	string pageContentFileRelativePath;

	//创建网页文件，此处使用路径名创建文件夹中的文件可避免进入文件夹
	pageContentFileRelativePath.append("./").append(destDirectory).append("/").append(pageContentFileName);
	fstream fsPageContent;
	
	//以只写方式打开文件，若文件不存在则创建该文件
	fsPageContent.open(pageContentFileRelativePath.c_str(),ios_base::out);
	
	if(!fsPageContent)
	{
		cout<<url<<" 文件创建错误\n";
		return -1;
	}
	else
	{
		fsPageContent.write(pageContent.c_str(),pageContent.size());
		fsPageContent.close();
	}

	//将该url存入文件夹下存储网址的文件中
	
	//每个线程添加时要锁住该文件
	pthread_mutex_lock(&m_saveUrlLogFileMutex);

	string saveUrlLogFileName = "save_url_list";         //列表文件名
	string savedUrlLogFileRelativePath;
	savedUrlLogFileRelativePath.append("./").append(destDirectory).append("/").append(saveUrlLogFileName);

	fstream fsSaveUrlLog;
	fsSaveUrlLog.clear();
	fsSaveUrlLog.close();
	fsSaveUrlLog.open(savedUrlLogFileRelativePath.c_str(),ios_base::out|ios_base::app);   //在文件末尾写入

	if(!fsSaveUrlLog)
	{
		cout<<"打开url列表文件错误"<<endl;
		fsSaveUrlLog.close();
		fsSaveUrlLog.clear();
		return -1;
	}
	else
	{
		fsSaveUrlLog<<url<<" "<<pageContentFileName<<" "<<pageContent.size()<<endl;
		fsSaveUrlLog.close();
		fsSaveUrlLog.clear();
	}

	pthread_mutex_unlock(&m_saveUrlLogFileMutex);

	return 0;
}

/********************************************
函数：crawler_extract_url
描述：获取网页中的所有网址
参数：
1、url：网址
2、pageContent：网页内容
3、externalUrlList：其他网站的地址列表
4、internalUrlList：本网站的链接文件
返回值：
int：获取的url数量
********************************************/
int Crawler::crawler_extract_url(const string& url,const string& pageContent,list<string>& externalUrlList,list<string>& internalUrlList)
{
	size_t posTmp;
	string strTmp;
	
	size_t posLinkMark;
	
	//某一网站地址的左右引号
	size_t posLeftQuotation,posRightQuotation;
	
	size_t posStart = 0;

	//href=与“”之间所允许的空格数
	const size_t numAllowedSpace = 2;

	size_t urlLen = 0;          //获取的url长度
	size_t urlCount = 0;        //共获取的url数量

	posTmp = url.find("://");
	if(posTmp == string::npos)
	{
		cout<<"网址错误"<<endl;
		return -1;
	}

//	cout<<"5555555555555555"<<endl;
//	int m;
//	cin>>m;

	//如果url中包含资源文件，则检查是不是常见的资源文件
	if( ( *(url.end()-1) != '/' ) &&( (posTmp = url.substr(posTmp+3).rfind('/')) != string::npos ) )
	{
		if((posTmp = (url.substr(posTmp+1).rfind("."))) != string::npos)
		{
			strTmp = url.substr(posTmp+1);    
			if(strTmp.size() <= 4)              //获取网页文件的后缀名
			{
				if(strTmp!=string("html") && strTmp != string("htm") && strTmp!=string("php") && strTmp!=string("jsp") && strTmp!=string("aspx") && strTmp!=string("asp") && strTmp!=string("cgi"))
				{
					cout<<"网页文件格式错误"<<endl;
					return -1;
				}
			}
		}
	}

	while(1)
	{

		//如果找不到href标志则退出
		if((posLinkMark = pageContent.find("href=",posStart)) == string::npos)
		{
		//	cout<<"无其他地址"<<endl;
			break;
		}

		posLeftQuotation = pageContent.find('\"',posLinkMark+5);      //查找左引号

		if(posLeftQuotation == string::npos)          //不是href="..."的形式
		{
			break;                               //退出
		}
		
		//href与“”之间的空格不能超过numAllowedSpace
		if( (posLeftQuotation - (posLinkMark+5) ) > numAllowedSpace )
		{
			posStart = posLinkMark + 5;
			continue;
		}

		posRightQuotation = pageContent.find('\"',posLeftQuotation+1);       //查找与左引号临近的右引号

		if(posRightQuotation == string::npos)
		{
			break;
		}

		if( (posRightQuotation - posLeftQuotation) == 1 )                //href=""的情况
		{
			posStart = posRightQuotation+1;
			continue;
		}

		string urlTmp;             //临时存储获取的url
		urlLen = (posRightQuotation-1) - (posLeftQuotation+1) + 1;     //获取的url长度，该长度应不包括左右引号
		urlTmp = pageContent.substr(posLeftQuotation+1,urlLen);

		//向列表中添加url
		string jointedUrl;
		int rtn = joint_url(url,urlTmp,jointedUrl);
		if(rtn == 0 )
		{
			internalUrlList.push_back(jointedUrl);
			//cout<<"获取一个站内资源："<<jointedUrl<<endl;
			++urlCount;
		}
		else if(rtn == 1)
		{
			externalUrlList.push_back(jointedUrl);
			//cout<<"获取一个其他站链接："<<jointedUrl<<endl;
			++urlCount;
		}
		
		posStart = posRightQuotation + 1;
	//	cout<<jointedUrl<<endl;

	}
//	cout<<"从"<<url<<"中一共获取"<<urlCount<<"个网址"<<endl;

	return urlCount;
	
}

/****************************************
函数：joint_url
描述：将给定的网址转化为标准的url地址
参数：
1、currentPageLink：当前页面链接
2、jumpPageLink：需要转化的网址
3、jointedUrl：存储转化后的标准网址
返回值：
1、int：若获得站内链接则返回0，若获得站外
   的其他网站则返回1 
****************************************/
int Crawler::joint_url(const string& currentPageLink,const string& jumpPageLink,string& jointedUrl)
{
	//忽略邮件、js可执行程序以及返回顶部等类型的链接
	if((jumpPageLink.find("mailto:")!=string::npos) || (jumpPageLink.find("javascript:")!=string::npos) || (jumpPageLink.find("#")!=string::npos))
	{
		return -1;
	}

	//查找其他网站链接
	if(jumpPageLink.find("://")!=string::npos)       //获取到一个完整的网址
	{	
		if(get_domain_from_url(jumpPageLink) != get_domain_from_url(currentPageLink))
		{
			//获得站外链接
			jointedUrl = jumpPageLink;
			return 1;
		}

		else
		{
			//获得站内资源
			jointedUrl = jumpPageLink;
			return 0;
		}

	}
	
//	cout<<"11111111111111111"<<endl;
//	int q;
//	cin>>q;

	//查找该网站的本地链接
	else
	{
		//该链接是从根目录开始的绝对路径
		if(*(jumpPageLink.begin()) == '/')
		{
			size_t posTmp = 0;

			//如果在原url中未找到“http://”则出错
			if((posTmp = currentPageLink.find("://")) == string::npos)
			{
				return -1;
			}

			//如果原url中无根目录标示“/”，则在原url的末尾加入该路径
			if((posTmp = currentPageLink.find("/",posTmp+3)) == string::npos)
			{
				jointedUrl.append(currentPageLink).append(jumpPageLink);
				return 0;
			}

			//如果原url带有根标示“/”,则在原url的第一个根标示“/”的前一个位置加入该路径

			else
			{
				jointedUrl.append(currentPageLink.substr(0,posTmp)).append(jumpPageLink);
				return 0;
			}
		}

		//该路径为链接至上级目录的相对路径，如href="../../intx.html"
		else if(*(jumpPageLink.begin()) == '.')
		{
			//搜索路径中“../”的数量，查看需要返回到哪一级的目录中
			size_t posTmp = 0, posStart = 0, backtrackDepth = 0;
			while(posTmp = jumpPageLink.find("../",posStart)!=string::npos)
			{
				++backtrackDepth;
				posStart = posTmp+3;
			}
	
			//进入原url的当前目录
			size_t posEnd = 0;
			posTmp = currentPageLink.rfind("/");   
			posEnd = posTmp-1;

			while(backtrackDepth>0)
			{
				posTmp=currentPageLink.rfind("/",posEnd);   //注意rfind是从右向左查找
				if( (posTmp == string::npos) || (currentPageLink[posTmp-1] == '/') || (currentPageLink[posTmp+1] == '/') )
				{
					return -1;
				}

				posEnd = posTmp-1;
				--backtrackDepth;
			}

			//在原url对应目录的后边加入该路径名,该路径名应去掉前面的多个“../”
			jointedUrl.append(currentPageLink.substr(0,posEnd+2)).append(jumpPageLink.substr(posStart));
			return 0;
		}
		
		//该链接为不带路径信息的当前目录中的文件，如href="link1.html"
		else
		{
			size_t posTmp = 0;

			//反向查找原url中是否有路径标示“/”，若没有则出错
			if((posTmp=currentPageLink.rfind('/')) == string::npos)
			{
				return -1;
			}
			
			//如果搜索到http://中的“/”，则在url末尾添加根目录“/”以及该文件
			if(currentPageLink[posTmp-1] == '/')
			{
				jointedUrl.append(currentPageLink).append("/").append(jumpPageLink);
				return 0;
			}

			//否则在末尾的“/”后面加入该路径名
			else
			{
				jointedUrl.append(currentPageLink.substr(0,posTmp+1)).append(jumpPageLink);
				return 0;
			}
		}
	}

}

/*****************************************
函数：get_domain_from_url
描述：获得url的域名
参数：
1、url：网址
返回值：
string：该网址对应的域名
*****************************************/
string Crawler::get_domain_from_url(const string& url)
{
	string protocol,domain,path;
	parse_url(url,protocol,domain,path);
	return domain;
}

/******************************************
函数：crawler_add_url
描述：将待搜索的url放入线程池
参数：
1、pool：线程池
2、urlList：待搜索url列表
返回值：存入线程池的url个数
******************************************/
int Crawler::crawler_add_url(ThreadPool *pool,const string& url)
{
	return pool->threadpool_add_job(url);
}

int Crawler::crawler_add_url(ThreadPool *pool,const list<string>& urlList)
{
	list<string>::const_iterator iter = urlList.begin();
	int rtn = 0;
	while(iter!=urlList.end())
	{
		rtn += crawler_add_url(pool,*iter);
		++iter;
	}

	return rtn;
}

int Crawler::crawler_add_url(const string &url)
{
	return m_crawlerThreadPool.threadpool_add_job(url);
}

/****************************************
函数：crawler_get_instance
描述：返回一个爬虫实例
参数：
1、threadNum：线程数
2、maxUrlQueueLen：url爬取队列的长度
返回值：
1、Crawler*：爬虫实例指针
****************************************/
Crawler* Crawler::crawler_get_instance(size_t threadNum,size_t maxUrlQueueLen)
{
	if(m_crawlerIsExited == true)
	{
		cout<<"爬虫实例已经存在，需要重新开启"<<endl;
		return NULL;
	}
	static Crawler crawlerInstance(crawler_main_loop,threadNum,maxUrlQueueLen);
	return &crawlerInstance;
}











