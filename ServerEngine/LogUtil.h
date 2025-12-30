#pragma once

class LogUtil
{
public:
	static void error_display(int err_no);
	static void error_display(const char* msg);
	static void PrintLog(const char* s);
};

