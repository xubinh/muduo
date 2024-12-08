// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include "muduo/base/noncopyable.h"
#include "muduo/net/InetAddress.h"

#include <functional>
#include <memory>

namespace muduo {
namespace net {

class Channel;
class EventLoop;

class Connector : noncopyable, public std::enable_shared_from_this<Connector> {
public:
    typedef std::function<void(int sockfd)> NewConnectionCallback;

    Connector(EventLoop *loop, const InetAddress &serverAddr);

    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback &cb) {
        newConnectionCallback_ = cb;
    }

    // 设置 `connect_ = true`, 并调用 `startInLoop()`
    void start(); // can be called in any thread

    void restart(); // must be called in loop thread
    void stop();    // can be called in any thread

    const InetAddress &serverAddress() const {
        return serverAddr_;
    }

private:
    enum States { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    void setState(States s) {
        state_ = s;
    }

    // 调用 `connect()`
    void startInLoop();

    void stopInLoop();

    // 创建非阻塞 socketfd, 调用 `::connect()`, 并视情况调用 `connecting()`,
    // `retry()`, 以及 `::close()`
    void connect();

    // 设置状态为 `kConnecting`, 首次创建 `Channel` 对象, 注册回调, 并启动监听
    // socketfd 的写事件
    void connecting(int sockfd);

    // 如果监听到了写事件则立即断开 `Channel` 连接, 然后视情况建立 TCP
    // 连接 (调用 `newConnectionCallback_`) 或者进行重试 (调用 `retry()`)
    void handleWrite();

    // 调用 `retry()`
    void handleError();

    void retry(int sockfd);

    int removeAndResetChannel();

    void resetChannel();

    EventLoop *loop_;
    InetAddress serverAddr_;
    bool connect_; // atomic
    States state_; // FIXME: use atomic variable
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDelayMs_;
};

} // namespace net
} // namespace muduo

#endif // MUDUO_NET_CONNECTOR_H
