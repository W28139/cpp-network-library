#pragma once
#include<string>
#include<iostream>

class Buffer
{
private:
	std::string buf_;
public:
	Buffer();
	~Buffer();
	
	void append(const char* data,size_t size);
	size_t size();
	void erase(size_t pos,size_t nn); 	// 从buf_的pos开始，删除nn个字节，pos从0开始
	const char* data();
	void clear();
};
