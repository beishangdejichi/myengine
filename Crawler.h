#ifndef _CRAWLER_H_
#define _CRAWLER_

#include <stdio.h>
#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <time.h>
#include "./ThreadPool.h"
#include <boost/thread.hpp>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include <fstream>

using namespace std;

class Crawler
{
	public:
		Crawler(void(*Func)(string,ThreadPool* pool),size_t threadNum,size_t maxUrlQueueLen);
		~Crawler();
		int crawler_exit();
		static void get_time_string(string& timeStr);
		static int crawler_crawl_page(const string& url, string& pageContent);
		static int parse_url(const string& url,string& protocol,string& domain,string& path);
		static void convert_url_to_file_name(const string& url,string& fileName);
		static void crawler_main_loop(string url,ThreadPool *pool);
		static int crawler_save_page(const string& destDirectory,const string& url,const string& pageContent);
		static int crawler_extract_url(const string& url,const string& pageContent,list<string>& externalUrlList,list<string>& internalUrlList);
		static int joint_url(const string& currentPageLink,const string& jumpPageLink,string& jointedUrl);
		static string get_domain_from_url(const string& url);
		static int crawler_add_url(ThreadPool *pool,const list<string>& urlList);
		static int crawler_add_url(ThreadPool *pool,const string& url);
		int crawler_add_url(const string &url);
		static Crawler* crawler_get_instance(size_t threadNum,size_t maxUrlQueueLen);
	private:
		//线程池变量
		ThreadPool m_crawlerThreadPool;

		//标记爬虫是否开启的标志位
		static bool m_crawlerIsExited;

		//向url目录文件中添加记录时的互斥量
		static pthread_mutex_t m_saveUrlLogFileMutex;

		//目录名称
		static string m_destDirectory;

		//目录中每个网页文件的文件名
		//string m_saveUrlLogFileName;

};

#endif
