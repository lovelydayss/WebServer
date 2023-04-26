#define FMTLOG_HEADER_ONLY
#include "../include/fmtlog/fmtlog.h"
#include <iostream>

int main() {
	// std::cout << "hello world!" << std::endl;
	fmtlogT<>::setLogLevel(fmtlogT<>::DBG);
	fmtlogT<>::setLogFile("./out/output.txt");
	fmtlogT<>::setHeaderPattern("{YmdHMSf} {s} {l}[{t}]  ");
	fmtlogT<>::closeLogFile();
	fmtlogT<>::setLogFile("./out/output.txt");
	logi("My name is {}, and I am {} years old.", "zhangyiwei", 22);
	loge("My name is {}, and I am {} years old.", "zhangyiwei", 22);
	return 0;
}	