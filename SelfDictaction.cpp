#include <sapi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <io.h>
#include <string>
#include <iomanip>
#include <direct.h>
#include <vector>
#include "CJsonObject.hpp"
#include<cstdlib>
#include<ctime>//for rundom function.

#define SD_APP_VERSION 0.1

//Random function
#define randomInt(a,b) (rand()%(b-a+1)+a)

using namespace std;

#pragma comment(lib,"ole32.lib") //CoInitialize CoCreateInstance需要调用ole32.dll   
#pragma comment(lib,"sapi.lib") //sapi.lib在SDK的lib目录,必需正确配置   

int SceenWidth = 120;
int SceenHeight = 40;

char   exe_path[MAX_PATH];

inline int SAPIInitial();

inline int load_data();

int dictaction(ISpVoice* pvoc, vector<neb::CJsonObject> vunit, int test_num);

int voc_check(ISpVoice* pvoc, const char* test_voc, const char* voc_meaning);

int voice(ISpVoice* pvoc, const char* str);

string readFileIntoString(char* filename);
ISpVoice* pVoice = NULL;

vector<neb::CJsonObject> unitLoad;
vector<neb::CJsonObject> unitTest;

int main(int argc, char* argv[]){

	//COM初始化：   
	if (FAILED(::CoInitialize(NULL)))return FALSE;

	//获取ISpVoice接口：   
	HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)& pVoice);
	//SUCCEEDED(hr)

	//GetCurrentDirectory(MAX_PATH, exe_path);
	_getcwd(exe_path, MAX_PATH);
	//cout << exe_path << endl;


	system("chcp 65001");
	cout << "Loading vocabulary data...\n";
	load_data();
	unitTest = unitLoad;
	int testNum = unitTest.size();
	//cout << testNum;
	cout << "============================================================\n";
	cout << "Welcome to the self-service dictation system!\n";
	cout << "Now it can only dictaction unit 3.\n";
	cout << "Enter \"run\" to start.\n";
	cout << "============================================================\n";

	char command[64];
	while (1) {
		cin >> command;
		if (!strcmp(command, "run")) {
			cout << "============================================================\n";
			cout << "Now you will hear 10 word and please write down them on your keyboard.\n";
			cout << "Tips:\n";
			cout << "    1.If you don't hear it clearly, you can type replay and listen again.\n";
			cout << "    2.In general, the words you type are all lowercase.\n";
			cout << "============================================================\n";
			dictaction(pVoice, unitTest, 10);
			cout << "Good job! You finished 10 vocabulary test!\n";
			cout << "You can type \"run\" to try again!\n";
			cout << "============================================================\n";

		}
		else {
			cout << "Command not found! Please try again.\n";
		}
	}



	pVoice->Release();
	pVoice = NULL;
	//千万不要忘记：   
	::CoUninitialize();
	return TRUE;
}

inline int SAPIInitial() {
	//COM初始化：   
	if (FAILED(::CoInitialize(NULL)))return -1;


}

string readFileIntoString(char* filename){
	ifstream ifile(filename);
	//将文件读入到ostringstream对象buf中
	ostringstream buf;
	char ch;
	while (buf && ifile.get(ch))
		buf.put(ch);
	//返回与流对象buf关联的字符串
	return buf.str();
}

inline int load_data() {
	strcat(exe_path, "/data/");
	strcat(exe_path, "*.*");
	intptr_t handle;
	_finddata_t findData;

	handle = _findfirst(exe_path, &findData);    // 查找目录中的第一个文件
	if (handle == -1){
		cout << "Failed to find first file!\n";
		return -1;
	}
	char filenameLast[100] = { 0 };
	do{
		if (findData.attrib & _A_SUBDIR
			|| strcmp(findData.name, ".") == 0
			|| strcmp(findData.name, "..") == 0)continue;
		char* tmp = strrchr(findData.name, '.');
		strcpy(filenameLast, tmp + 1);
		if (!strcmp(filenameLast, "json")) {
			cout << findData.name << endl;
			char tmp_path[260];
			strcpy(tmp_path, "data/");
			strcat(tmp_path, findData.name);
			string tmpJson = readFileIntoString(tmp_path);
			neb::CJsonObject tmpUnit(tmpJson.c_str());
			unitLoad.push_back(tmpJson);
		}
	} while (_findnext(handle, &findData) == 0);    // 查找目录中的下一个文件

	cout << "Done!\n";
	_findclose(handle);    // 关闭搜索句柄

	return 0;
}

int dictaction(ISpVoice* pvoc, vector<neb::CJsonObject> vunit, int test_num) {
	srand((unsigned)time(NULL));
	for (int i = 0; i < test_num; i++) {
		int test_unit = rand() % vunit.size();
		int test_word = rand() % vunit[test_unit]["vocabulary"].GetArraySize();
		//debug;
		//cout << test_unit << test_word << endl;
		string tsWord, tsMean;//tmp string word&mean.
		vunit[test_unit]["vocabulary"][test_word].Get("word", tsWord);
		vunit[test_unit]["vocabulary"][test_word].Get("meaning", tsMean);
		voc_check(pvoc, tsWord.c_str(), tsMean.c_str());
		Sleep(2000);
		cout << "========== N e x t ==========\n";
	}

	return 0;
}

int voc_check(ISpVoice* pvoc, const char* test_voc, const char* voc_meaning) {
	bool loop;
	int wrong_times = 0;
	do {
		loop = false;
		voice(pvoc, test_voc);
		cout << "Please input the vocabulary:";
		char tmpvoc[64];
		scanf("%s", tmpvoc);
		if (!strcmp(tmpvoc, "replay"))loop = true;
		else if (!strcmp(tmpvoc, test_voc)) {
			cout << "Right! Now write down the meaning on your paper, then press enter key.\n";
			system("pause");
			cout << "The meaning of the word:\n";
			cout << voc_meaning << endl;
		}
		else {
			wrong_times++;
			if (wrong_times < 4) {
				cout << "Wrong, please try again!\n";
				loop = true;
			}
			else {
				cout << "You wrong too many times! The answer is " << test_voc << "!\n";
				loop = true;
			}
		}

	} while (loop);
	return 0;
}

int voice(ISpVoice* pvoc, const char* str) {
	WCHAR tmp_wchar[64];
	LPCWSTR tmp_lpcwstr = tmp_wchar;
	MultiByteToWideChar(0, 0, str, -1, tmp_wchar, 64);
	pvoc->Speak(tmp_lpcwstr, 0, NULL);
	//cout << tmp_lpcwstr << endl;
	return 0;
}

/*
print("====Self-service dictation system by Panedioic. -ver 1.0 ====")
print("change #num -> Change unit settion.")
print("set #num -> change default num.")
print("run -> run with last setting.")
print("all -> run with all unit.")
print("========================================")
while 1:
	option = input('Please enter your choice: ')
	#print(option)
	if option != 'run':
		print("Option not support now!")
		continue
*/