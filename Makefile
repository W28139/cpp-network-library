# ===============================
# 项目名称（可执行文件）
# ===============================
TARGET_SERVER = echoserver
TARGET_CLIENT = client

# ===============================
# 编译器 & 编译选项
# ===============================
CXX = g++
CXXFLAGS = -g -Wall -std=c++11
LDFLAGS = -lpthread

# ===============================
# 源文件（统一管理，方便扩展）
# ===============================
SERVER_SRCS = echoserver.cpp \
              InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp \
              EventLoop.cpp TcpServer.cpp Acceptor.cpp \
              Connection.cpp Buffer.cpp EchoServer.cpp \
              ThreadPool.cpp Timestamp.cpp

CLIENT_SRCS = client.cpp

# ===============================
# 目标文件（.cpp → .o）
# 自动替换规则（很重要）
# ===============================
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# ===============================
# 默认目标（make 时执行）
# ===============================
all: $(TARGET_CLIENT) $(TARGET_SERVER)

# ===============================
# 生成 server
# ===============================
$(TARGET_SERVER): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# ===============================
# 生成 client
# ===============================
$(TARGET_CLIENT): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ===============================
# 通用编译规则（核心优化点🔥）
# 把 .cpp 编译成 .o（避免重复编译）
# ===============================
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ===============================
# 清理
# ===============================
clean:
	rm -f *.o $(TARGET_CLIENT) $(TARGET_SERVER)

# ===============================
# 伪目标（避免冲突）
# ===============================
.PHONY: all clean
