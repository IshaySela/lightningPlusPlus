#include "lightning/httpServer/NonblockingClientManagerTask.hpp"
#include "lightning/httpServer/HttpServer.hpp"
#include <cerrno>
#include <iostream>
#include <string_view>
#include <sys/epoll.h>
#include <unistd.h>

namespace lightning
{
    NonblockingClientManagerTask::NonblockingClientManagerTask(
        NewFdChannel& newFdChannel,
        ReturnChannel& returnChannel,
        int threadCount,
        std::reference_wrapper<HttpServer> server
    ) : newFdChannel(newFdChannel),
        returnChannel(returnChannel),
        workerPool(threadCount),
        server(server)
    {}

    auto NonblockingClientManagerTask::operator()() -> void
    {
        epollFd = epoll_create1(EPOLL_CLOEXEC);

        addToEpoll(newFdChannel.pipeRead, EPOLLIN);
        addToEpoll(returnChannel.pipeRead, EPOLLIN);

        std::array<epoll_event, 64> events;

        while (true)
        {
            int n = epoll_wait(epollFd, events.data(), events.size(), -1);
            if (n < 0)
            {
                if (errno == EINTR) continue;  // signal interrupted the wait - retry
                std::cerr << "epoll_wait fatal error: " << strerror(errno) << '\n';
                break;
            }

            for (int i = 0; i < n; i++)
            {
                int fd = events[i].data.fd;
                uint32_t ev = events[i].events;

                if (fd == newFdChannel.pipeRead)
                {
                    char buf[256];
                    ssize_t r = read(fd, buf, sizeof(buf));
                    if (r == 0)  // write end closed — shutdown signal
                    {
                        close(epollFd);
                        return;
                    }
                    drainNewFdChannel();
                }
                else if (fd == returnChannel.pipeRead)
                {
                    char buf[256];
                    read(fd, buf, sizeof(buf));
                    drainReturnChannel();
                }
                else
                {
                    handleClientEvent(fd, ev);
                }
            }
        }

        close(epollFd);
    }

    auto NonblockingClientManagerTask::addToEpoll(int fd, uint32_t events) -> void
    {
        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;
        epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
    }

    auto NonblockingClientManagerTask::rearmFd(int fd) -> void
    {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLONESHOT;
        ev.data.fd = fd;
        epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
    }

    auto NonblockingClientManagerTask::drainNewFdChannel() -> void
    {
        std::vector<std::unique_ptr<IClient>> newClients;
        {
            std::lock_guard lock(newFdChannel.m);
            newClients = std::move(newFdChannel.clients);
            newFdChannel.clients.clear();
        }

        for (auto& client : newClients)
        {
            int fd = client->getFd();
            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLONESHOT;
            ev.data.fd = fd;
            epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
            connections.emplace(fd, ConnectionState{std::move(client)});
        }
    }

    auto NonblockingClientManagerTask::drainReturnChannel() -> void
    {
        std::vector<ReturnedConnection> returned;
        {
            std::lock_guard lock(returnChannel.m);
            returned = std::move(returnChannel.connections);
            returnChannel.connections.clear();
        }

        for (auto& rc : returned)
        {
            if (rc.keepAlive)
            {
                int fd = rc.client->getFd();
                connections.emplace(fd, ConnectionState{std::move(rc.client)});
                rearmFd(fd);
            }
            else
            {
                rc.client->getStream().close();
            }
        }
    }

    auto NonblockingClientManagerTask::handleClientEvent(int fd, uint32_t events) -> void
    {
        auto it = connections.find(fd);
        if (it == connections.end()) return;

        auto& state = it->second;

        if (events & (EPOLLERR | EPOLLHUP))
        {
            state.client->getStream().close();
            connections.erase(it);
            return;
        }

        auto& stream = state.client->getStream();
        auto view = stream.peek(MAX_HEADER_BYTES);
        auto pos = view.find("\r\n\r\n");

        if (pos == std::string_view::npos)
        {
            rearmFd(fd);
            return;
        }

        // Consume headers + terminator from the stream, leaving any body data in the stream's buffer.
        auto consumed = stream.read(static_cast<int>(pos + 4));
        std::vector<char> headerData(consumed.begin(), consumed.begin() + pos);

        auto client = std::move(state.client);
        connections.erase(it);

        workerPool.add_task(ClientRequestHandler(
            std::move(client),
            std::move(headerData),
            server,
            returnChannel
        ));
    }
} // namespace lightning
