#include "libevent.h"

#include "msg_process.h"
#include "protocol/version.h"


namespace btclite {
namespace network {

struct event_base *LibEvent::EventBaseNew()
{
    return event_base_new();
}

int LibEvent::EventBaseDispatch(struct event_base *event_base)
{
    if (event_base)
        return event_base_dispatch(event_base);
    return -1;
}

void LibEvent::EventBaseFree(struct event_base *base)
{
    if (base)
        event_base_free(base);
}

int LibEvent::EventBaseLoopexit(struct event_base *event_base, const struct timeval *tv)
{
    if (event_base)
        return event_base_loopexit(event_base, tv);
    return -1;
}

struct bufferevent *LibEvent::BuffereventSocketNew(struct event_base *base, evutil_socket_t fd, int options)
{
    if (base)
        return bufferevent_socket_new(base, fd, options);
    return nullptr;
}

void LibEvent::BuffereventSetcb(struct bufferevent *bev, bufferevent_data_cb readcb,
                                 bufferevent_data_cb writecb, bufferevent_event_cb eventcb, void *cbarg)
{
    if (bev)
        bufferevent_setcb(bev, readcb, writecb, eventcb, cbarg);
}

int LibEvent::BuffereventEnable(struct bufferevent *bev, short event)
{
    if (bev)
        return bufferevent_enable(bev, event);
}

struct evconnlistener *LibEvent::EvconnlistenerNewBind(struct event_base *base, evconnlistener_cb cb,
                                                         void *ptr, unsigned flags, int backlog,
                                                         const struct sockaddr *sa, int socklen)
{
    if (base)
        return evconnlistener_new_bind(base, cb, ptr, flags, backlog, sa, socklen);
    
    return nullptr;
}

void LibEvent::EvconnlistenerSetErrorCb(struct evconnlistener *lev, evconnlistener_errorcb cb)
{
    if (lev)
        evconnlistener_set_error_cb(lev, cb);
}

Socket::Fd LibEvent::EvconnlistenerGetFd(struct evconnlistener *lev)
{
    if (lev)
        return evconnlistener_get_fd(lev);
}

struct event_base *LibEvent::EvconnlistenerGetBase(struct evconnlistener *lev)
{
    if (lev)
        return evconnlistener_get_base(lev);
}

void LibEvent::EvconnlistenerFree(struct evconnlistener *lev)
{
    if (lev)
        evconnlistener_free(lev);
}

bool Context::IsValid() const
{
    return (pparams && plocal_service && pnodes &&
            pchain_state && pbanlist && ppeers);
}

void ConnReadCb(struct bufferevent *bev, void *ctx)
{
    auto *context = reinterpret_cast<struct Context*>(ctx);
    if (!context || !context->pnodes || !context->pparams) {
        BTCLOG(LOG_LEVEL_WARNING) << "Pass nullptr to " << __func__;
        return;
    }
    
    auto pnode = context->pnodes->GetNode(bev);
    if (!pnode) {
        return;
    }
    
    if (pnode->connection().socket_no_msg()) {
        pnode->mutable_connection()->set_socket_no_msg(false);
    }
    
    if (pnode->timers().no_receiving_timer) {
        pnode->mutable_timers()->no_receiving_timer->Reset();
    }
    else {
        util::TimerMng& timer_mng = util::SingletonTimerMng::GetInstance();
        uint32_t timeout = (pnode->protocol().version > kBip31Version) ? 
                           kNoReceivingTimeoutBip31 : kNoReceivingTimeout;
        pnode->mutable_timers()->no_receiving_timer = 
            timer_mng.StartTimer(timeout*1000, 0, 
                                 std::bind(&Node::InactivityTimeoutCb, pnode));
    }
    
    auto task = std::bind(ParseMsg, pnode, std::ref(*(context->pparams)), 
                          std::ref(*(context->plocal_service)), context->ppeers,
                          context->pchain_state);
    util::SingletonThreadPool::GetInstance().AddTask(std::function<bool()>(task));
}

void ConnEventCb(struct bufferevent *bev, short events, void *ctx)
{
    auto *context = reinterpret_cast<struct Context*>(ctx);
    auto pnode = context->pnodes->GetNode(bev);
    if (!pnode)
        return;
    
    if (events & BEV_EVENT_CONNECTED) {
        
    }
    else if (events & BEV_EVENT_EOF) {
        if (!pnode->connection().IsDisconnected()) {
            BTCLOG(LOG_LEVEL_WARNING) << "peer " << pnode->id() 
                                      << " socket closed";
            pnode->mutable_connection()->Disconnect();
        }
    }
    else if (events & BEV_EVENT_ERROR) {
        if (errno != EWOULDBLOCK && errno != EMSGSIZE && 
                errno != EINTR && errno != EINPROGRESS)
        {
            if (!pnode->connection().IsDisconnected()) {
                BTCLOG(LOG_LEVEL_ERROR) << "peer " << pnode->id() << " socket recv error:"
                                        << std::string(strerror(errno));
                pnode->mutable_connection()->Disconnect();
            }
            
        }
    }
}

} // namespace network
} // namespace btclite
