#include"my_muduo/Buffer.h"

// private:
//        std::string buf_;

Buffer::Buffer(uint16_t seq):seq_(seq)
{}

Buffer::~Buffer(){}

void Buffer::append(const char* data,size_t size)
{
	buf_.append(data,size);
}

size_t Buffer::size()
{
	return buf_.size();
}

const char* Buffer::data()
{
	return buf_.data();
}

void Buffer::clear()
{
	buf_.clear();
}

void Buffer::erase(size_t pos,size_t nn)
{
	buf_.erase(pos,nn);
}

void Buffer::appendwithseq(const char *data,size_t size)
{
	if(seq_ == 0)
	{
		buf_.append(data,size);
	}
	else if(seq_ == 1)
	{
		buf_.append((char*)&size,4);
		buf_.append(data,size);
	}
	else if(seq_ == 2)
	{
		// 按需要完善
	}
}


bool Buffer::pickmessage(std::string &ss)
{
	if(buf_.size()==0)
	{
		return false;
	}
	if(seq_==0)
	{
		ss = buf_;
		buf_.clear();
		
	}
	else if(seq_ == 1)
	{
		int len;
		memcpy(&len,buf_.data(),4);
		if(buf_.size()<len+4)
		{
			return 0;
		}
		ss = buf_.substr(4,len);
		buf_.erase(0,len+4);
	}
	return true;
}
			
