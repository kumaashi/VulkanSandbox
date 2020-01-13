#pragma once

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>


#pragma  comment(lib, "gdi32.lib")
#pragma  comment(lib, "winmm.lib")
#pragma  comment(lib, "user32.lib")


void DoFind(const std::string dirname, const std::string wildcard, std::vector<std::string> &ret)
{
	using namespace std;
	HANDLE          hFind = 0;
	WIN32_FIND_DATA fd    = {0};
	const string basename = dirname + "/";
	const string findname = basename + wildcard;
	hFind = FindFirstFile(findname.c_str(), &fd);
	if(hFind == INVALID_HANDLE_VALUE) {
		return;
	}

	do {
		string fullfilename = string(basename) + string(fd.cFileName);
		if(string(".") == string(fd.cFileName))  continue;
		if(string("..") == string(fd.cFileName)) continue;
		ret.push_back(fullfilename);
		if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			string dname = basename + string(fd.cFileName) + "/";
			DoFind(dname, wildcard, ret);
		}
	} while(FindNextFile(hFind, &fd));
	FindClose(hFind);
}

void CreateProcessSync(char *szFileName, int sleepms = 5) {
	PROCESS_INFORMATION pi;
	STARTUPINFO si = {};
	si.cb = sizeof(si);
	if (CreateProcess(NULL, (LPTSTR)szFileName, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) {
		while (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0) {
			Sleep(sleepms);
		}
	}
}

std::string GetShaderBinaryName(std::string filename) {
	return filename + ".spv";
}

void CompileShader(const char *filename, const char *optionstr = "-V -o", const char *compilername = "glslangValidator.exe") {
	std::string infile   = std::string(filename);
	std::string outfile  = GetShaderBinaryName(filename);
	std::string compiler = std::string(compilername);
	std::string option   = std::string(optionstr);
	std::string command  = compiler + " " + option + " " + outfile + " " + infile;
	printf("%s:command=%s\n", __FUNCTION__, command.c_str());
	CreateProcessSync((char *)command.c_str());
}


int main() {
	std::vector<std::string> files;
	DoFind("./shader", "*", files);
	if(!files.empty()) {
		for(auto &x : files) {
			printf("%s\n", x.c_str());
			if(x.find(".spv") == std::string::npos) {
				CompileShader(x.c_str());
			}
		}
	}
	return 0;
}
