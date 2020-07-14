#include <iostream>
#include <string>
#include "base64.h"
using namespace std;

int main()
{
	const string ma64 = "RjUyMUVCNDlCRDhFOTk5QzVFOTk0RjAyQTYwOUQwNEIsMSwxMzYxMzYxMDAwMywwLDEsMCww";
	string s = base64_decode(ma64, false);
	cout << s << endl;
	std::string delimiter = ",";
	string ma64_new;

	size_t pos = 0;
	std::string token;
	pos = s.find(delimiter);
	token = s.substr(0, pos);
	ma64_new = token;
	std::cout << token << std::endl;
	s.erase(0, pos + delimiter.length());
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		token = s.substr(0, pos);
		std::cout << token << std::endl;
		s.erase(0, pos + delimiter.length());
	}
	std::cout << s << std::endl;

	// string ma_code ="528757552FF9AB3C8C5881B8DFE81739,1,13613610003,5,0,0,0";

	string ma_code =",1,13613610003,5,0,0,0";
	ma64_new.append(ma_code);
	cout << ma64_new <<endl;
	string s_encode = base64_encode(ma64_new, false);
	cout << s_encode << endl;
	return 0;
}
// g++ test_base64.cpp base64.cpp -o test