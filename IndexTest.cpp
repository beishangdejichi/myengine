#include "./Indexmodel.h"

using namespace std;

string Key_Word;
bool m_matchWordIndex;
bool isPureEnglish;

int main()
{
	cout<<"是否进行关键词搜索： y/n";
	char index;
	cin>>index;
	if(index == 'y')
	{
		m_matchWordIndex = true;
		cout<<"纯英文搜索请输入：e   中文搜索请输入：c"<<endl;
		char lag;
		cin>>lag;
		if(lag == 'e')
		{
			isPureEnglish = true;
		}
		else
		{
			isPureEnglish = false;
		}

		cout<<"请输入关键词："<<endl;
		string keyword;
		cin>>keyword;
		Key_Word = keyword;
	}
	else
	{
		m_matchWordIndex = false;
	}

	Crawler* pCrawler = Crawler::crawler_get_instance(20,15000);

	pCrawler->crawler_add_url("http://www.ew.com/ew/");
//	pCrawler->crawler_add_url("http://www.bad-url.net/");

	while(1);

	return 0;
}
