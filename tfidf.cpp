#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <boost/tokenizer.hpp>
#include <sys/stat.h>
#include <fcntl.h>
#include <utility>
#include <boost/lexical_cast.hpp>
#include <math.h>

using namespace std;


int findhtmltag(const string & fileContent,string& tag,int position);

int main()
{
	cout<<"生成训练集..."<<endl;
	map<string,int> TestWordList;
	DIR *dp;
	struct dirent *filename;
	dp = opendir("./test1");

	if(dp == NULL)
	{
		return -1;
	}	
	
	while((filename = readdir(dp)))
	{
		if((strncmp(filename->d_name,".",1)==0) || (strncmp(filename->d_name,"..",2)==0) )
		{
			continue;
		}
		else if(memchr(filename->d_name,'~',strlen(filename->d_name)))
		{
			continue;
		}
		else
		{
			string path = "./test1/";
			path.append(filename->d_name);

			ifstream fspagecontent(path.c_str());
			string buf;
			string text;
			while(getline(fspagecontent,buf))
			{
				text.append(buf);
			}
			
			//提取网页的内容，并进行分词

			string fileContent = text;
			
			string standardtext;       //存储剔除标签后的内容

			string tagBefore;   //存储前一个标签
			string tagAfter;    //存储后一个标签
			size_t posTagBefore;  //存储前一个标签的起始位置，即“<”的位置
			size_t posTagAfter;   //存储后一个标签的起始位置，即“<”的位置
			size_t posStart = 0;  //每次查询标签的起始位置
	
			//查找第一个标签

	   		//若无第一个标签或只有标签则出错
			if((posTagBefore = findhtmltag(fileContent,tagBefore,posStart)) == string::npos)
			{
				return -1;
			}

			posStart = posTagBefore + tagBefore.size(); //修改查找标签的起始位置，此时正好位于第一个标签的">"之后的第一个字符

			//依次查找后一个标签，直到文件尾部的</html>
			while((posTagAfter = findhtmltag(fileContent,tagAfter,posStart))!=string::npos)
			{
				//如果两个标签之间的正文字数大于1，并且不是script或style的执行程序，则提取标签之间的正文内容
				//其中script标签之间是js的执行代码，而style之间则是文档的样式信息
				if((posTagAfter-posTagBefore-tagBefore.size() >= 1) && (tagBefore.find("<script") == string::npos) && (tagBefore.find("<style") == string::npos))
				{
					string textSegment = fileContent.substr(posTagBefore+tagBefore.size(),posTagAfter - posTagBefore - tagBefore.size());
					standardtext.append(textSegment).append(" ");     //每段正文之间以空格分隔

				}

				posTagBefore = posTagAfter;
				tagBefore = tagAfter;
				posStart = posTagBefore + tagBefore.size();         //修改查找标签的起始位置，此时正好位于第一个标签的">"之后的第一个字符 
			}

			if(standardtext.size() == 0)
			{
				cout<<"网页内容剔除标签错误"<<endl;
			}
			
			//对网页内容进行分词和去冗余存储

			typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
			boost::char_separator<char> sep("?;:!&#%,.-|@/\\\"\"\'\' \n	");
			tokenizer tokens(standardtext,sep);

			set<string> WordList;

			for(tokenizer::iterator tok_iter = tokens.begin();tok_iter!=tokens.end();++tok_iter)
			{
				if(!WordList.count(*tok_iter))
				{
					WordList.insert(*tok_iter);
				}
			}

			for(set<string>::iterator set_iter = WordList.begin();set_iter!=WordList.end();++set_iter)
			{
				map<string,int>::iterator it = TestWordList.find(*set_iter);
				if(it!=TestWordList.end())
				{
					++(it->second);
				}
				else
				{
					TestWordList.insert(make_pair(*set_iter,1));
				}
			}

		}		
	
	}

	string StrResult;

	for(map<string,int>::iterator iter = TestWordList.begin();iter != TestWordList.end(); ++iter)
	{
		string sec = boost::lexical_cast<string>(iter->second);

		StrResult.append(iter->first).append(" ").append(sec).append("#");
	}

	//保存训练集
	string TestResult;
	TestResult.append("./").append("TestFile");
	fstream fsPageContent;
	fsPageContent.open(TestResult.c_str(),ios_base::out);
	if(!fsPageContent)
	{
		cout<<"文件创建错误"<<endl;
	}
	fsPageContent.write(StrResult.c_str(),StrResult.size());
	fsPageContent.close();
	
	cout<<"训练集生成完毕！"<<endl;
/*

	//将训练集中的数据存入字符串
	map<string,int> MatchWordList;
	string textpath = "./TestFile";
	ifstream testfilecontent(textpath.c_str());
	string textbuf;
	string matchword;
	while(getline(testfilecontent,textbuf))
	{
		matchword.append(textbuf);
	}

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> test_sep("#");
	tokenizer test_tokens(matchword,test_sep);


	for(tokenizer::iterator tok_iter = test_tokens.begin();tok_iter!=test_tokens.end();++tok_iter)
	{
		string temp = *tok_iter;
		boost::char_separator<char> match_sep(" ");
		tokenizer match_tokens(temp,match_sep);

		tokenizer::iterator match_iter = match_tokens.begin();
		string key = *match_iter;
		++match_iter;
		int value = boost::lexical_cast<int>(*match_iter);
		MatchWordList.insert(make_pair(key,value));
	}

//	for(map<string,int>::iterator map_it = MatchWordList.begin();map_it!=MatchWordList.end();++map_it)
//	{
//		cout<<map_it->first<<"  "<<map_it->second<<endl;
//	}
	
	string KeyWord = "This is a test";

	map<string,float> Key_Word_List;

	boost::char_separator<char> matchkeyword_sep(" ");
	tokenizer matchkeyword_tokens(KeyWord,matchkeyword_sep);

	int KeyWordNum = 0;
	for(tokenizer::iterator match_iter = matchkeyword_tokens.begin();match_iter!=matchkeyword_tokens.end();++match_iter)
	{
		++KeyWordNum;
	}

//	cout<<KeyWordNum<<endl;

	for(tokenizer::iterator match_iter = matchkeyword_tokens.begin();match_iter!=matchkeyword_tokens.end();++match_iter)
	{
		map<string,float>::iterator KeyWordList_iter = Key_Word_List.find(*match_iter);
		if(KeyWordList_iter != Key_Word_List.end())
		{
			KeyWordList_iter->second =float((float((KeyWordList_iter->second)*KeyWordNum+1))/(float(KeyWordNum)));
			//cout<<"second is "<<KeyWordList_iter->second<<endl;
		}
		else
		{
			float temvalue =float((float(1))/(float(KeyWordNum)));
			Key_Word_List.insert(make_pair(*match_iter,temvalue));
		}
	}

	int TestFileNum = 100;         //训练集总文档数

	//计算TF-IDF

	map<string,float> Word_TFIDF_List;

	for(map<string,float>::iterator Word_iter = Key_Word_List.begin();Word_iter != Key_Word_List.end();++Word_iter)
	{
		string tempstring = Word_iter->first;
		map<string,int>::iterator mat_iter = MatchWordList.find(Word_iter->first);
		float tempvalue;
		if(mat_iter != MatchWordList.end())
		{
			float t=float(log((float(100))/(float(mat_iter->second))));
			tempvalue=t*(float(Word_iter->second));


		}
		else
		{
			float t = float(log(float(100)));
			tempvalue = float(t*(float(Word_iter->second)));
		}

		Word_TFIDF_List.insert(make_pair(tempstring,tempvalue));
	}

	//找出最大权值的词语
	int Max = -1;
	set<string> KeyWords;
	for(map<string,float>::iterator tfidf_iter = Word_TFIDF_List.begin();tfidf_iter!=Word_TFIDF_List.end();++tfidf_iter)
	{
		if((tfidf_iter->second) > Max)
		{
			KeyWords.clear();
			KeyWords.insert(tfidf_iter->first);
			Max = tfidf_iter->second;
		}
		else if((tfidf_iter->second) = Max)
		{
			KeyWords.insert(tfidf_iter->first);
		}
	}

	

	for(set<string>::iterator find_iter = KeyWords.begin();find_iter!=KeyWords.end();++find_iter)
	{
		cout<<*find_iter<<endl;
	}


*/	

	return 0;
}


int findhtmltag(const string & fileContent,string& tag,int position)
{
	int tagStart,tagEnd;
	if((tagStart = fileContent.find("<",position)) == string::npos)
	{
		return string::npos;
	}
	if((tagEnd = fileContent.find(">",tagStart+1)) == string::npos)
	{
		return string::npos;
	}

	tag = fileContent.substr(tagStart,tagEnd-tagStart+1);
	return tagStart;
}
