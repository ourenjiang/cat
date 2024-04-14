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

namespace serial
{

using EventLoop = myself::reactor::EventLoop;
using Channel = myself::reactor::Channel;

class SerialConnector;
class SerialConnection;
typedef std::shared_ptr<SerialConnector> ConnectorPtr;
typedef std::shared_ptr<SerialConnection> ConnectionPtr;
typedef std::function<void (const ConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const ConnectionPtr&, Buffer*)> MessageCallback;
typedef std::function<void (const ConnectionPtr&)> CloseCallback;

}//namespace domain
} // namespace myself
