#include "my_muduo/Epoll.h"
#include "my_muduo/Channel.h"

#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <errno.h>

namespace mymuduo
{

Epoll::Epoll()
{
    // 使用 EPOLL_CLOEXEC 防止子进程继承该 fd
    epollfd_ = ::epoll_create1(EPOLL_CLOEXEC);
    if (epollfd_ < 0)
    {
        ::fprintf(stderr, "epoll_create error: %d\n", errno);
    }
}

Epoll::~Epoll()
{
    ::close(epollfd_);
}

void Epoll::poll(int timeoutMs, ChannelList* activeChannels)
{
    // 调用 epoll_wait
    int numEvents = ::epoll_wait(epollfd_, events_, kMaxEvents, timeoutMs);
    int savedErrno = errno;

    if (numEvents > 0)
    {
        for (int i = 0; i < numEvents; ++i)
        {
            // 通过 data.ptr 拿回 Channel 对象
            Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->setRevents(events_[i].events);
            // 填充给 EventLoop 供后续 handleEvent 处理
            activeChannels->push_back(channel);
        }
    }
    else if (numEvents == 0)
    {
        // timeout, do nothing
    }
    else
    {
        if (savedErrno != EINTR)
        {
            ::fprintf(stderr, "epoll_wait error: %d\n", savedErrno);
        }
    }
}

void Epoll::updateChannel(Channel* ch)
{
    struct epoll_event ev;
    ::memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;

    int fd = ch->fd();

    if (!ch->inEpoll())
    {
        // 如果不在 epoll 树上，执行 ADD
        if (::epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) < 0)
        {
            ::perror("epoll_ctl add error");
        }
        else
        {
            ch->setInEpoll(true);
        }
    }
    else
    {
        // 如果已经在树上，执行 MOD (或者删除，取决于 events)
        if (ch->isNoneEvent())
        {
            if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, 0) < 0)
            {
                ::perror("epoll_ctl del error");
            }
            ch->setInEpoll(false);
        }
        else
        {
            if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ev) < 0)
            {
                ::perror("epoll_ctl mod error");
            }
        }
    }
}

void Epoll::removeChannel(Channel* ch)
{
    int fd = ch->fd();
    if (ch->inEpoll())
    {
        if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, 0) < 0)
        {
            ::perror("epoll_ctl del error");
        }
        ch->setInEpoll(false);
    }
}

} // namespace mymuduo