#pragma once
#include<string>
#include<iostream>
#include<cstring>
#include <cstdint>

class Buffer
{
private:
	std::string buf_;
	const uint16_t seq_;	// 报文的分隔符，0. 无分隔符，1. 四字节分隔符，2. "\r\n\r\n"分隔符（http协议）
public:
	Buffer(uint16_t seq = 1);
	~Buffer();
	
	void append(const char* data,size_t size);
	void appendwithseq(const char* data,size_t size);
	size_t size();
	void erase(size_t pos,size_t nn); 	// 从buf_的pos开始，删除nn个字节，pos从0开始
	const char* data();
	void clear();

	bool pickmessage(std::string &ss);	// 从buf_中拆分一个报文，存放在ss中，如果buf_中没有报文，返回false
};
