#pragma once
#include "myself/reactor/Callbacks.h"
#include <functional>
#include <memory>

namespace myself
{

class Buffer;

namespace reactor
{
class EventLoop;
class Channel;
} //namespace reactor


namespace tcp
{

using EventLoop = myself::reactor::EventLoop;
using Channel = myself::reactor::Channel;

class Connector;
class TcpConnection;
typedef std::shared_ptr<Connector> ConnectorPtr;
typedef std::shared_ptr<TcpConnection> ConnectionPtr;
typedef std::function<void (const ConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const ConnectionPtr&, Buffer*)> MessageCallback;
typedef std::function<void (const ConnectionPtr&)> CloseCallback;

}//namespace tcp
} // namespace myself
