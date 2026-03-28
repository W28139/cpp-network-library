# ================= 编译器配置 =================
CXX = g++
CXXFLAGS = -g -Wall -std=c++11 -I./muduo
LDFLAGS = -lpthread

# ================= 目标程序 =================
TARGET = echoserver

# ================= 源文件 =================
SRCS = example/echoserver.cpp \
       muduo/InetAddress.cpp \
       muduo/Socket.cpp \
       muduo/Epoll.cpp \
       muduo/Channel.cpp \
       muduo/EventLoop.cpp \
       muduo/TcpServer.cpp \
       muduo/Acceptor.cpp \
       muduo/Connection.cpp \
       muduo/Buffer.cpp \
       muduo/EchoServer.cpp \
       muduo/ThreadPool.cpp \
       muduo/Timestamp.cpp

# ================= 目标文件 =================
OBJS = $(SRCS:.cpp=.o)

# ================= 默认目标 =================
all: $(TARGET)

# ================= 链接 =================
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# ================= 编译规则 =================
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ================= 清理 =================
clean:
	rm -f $(TARGET) $(OBJS)

# ================= 伪目标 =================
.PHONY: all clean
