#ifndef __UTILS_H
#define __UTILS_H

#include <cmath>
#include <io.h>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

const float EPS = 1e-6;


static bool isEqualf(float a, float b) {
	if (fabs(a - b) < EPS)
	{
		return true;
	}
	return false;

};

static vector<string> getListFiles(string path, string suffix) {
	_finddata_t file;
	intptr_t handle;

	string file_type;
	if (path.substr(0, 1) == "." || path.substr(0, 2) == "..")file_type = path + "/" + suffix;
	else file_type = path + "\\" + suffix;
	const char* cfile_type = file_type.c_str();

	//遍历当前文件夹中的文件
	int k = handle = _findfirst(cfile_type, &file);
	int i = 0;
	if (k == -1)
	{
		cout << "不存在" + suffix << endl;
		system("pause");
		exit(-1);
	}
	vector<string> filenames;
	while (k != -1)
	{
		filenames.push_back(string(file.name));
		k = _findnext(handle, &file);
	}
	_findclose(handle);
	return filenames;
};

#endif