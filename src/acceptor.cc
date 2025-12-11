/**
 * @file acceptor.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Acceptor" which is the acceptor of new TCP
 * connection requests from clients and create the connections.
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "acceptor.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

#include "logger.h"
#include "net_address.h"
#include "poller.h"

namespace taotu {
namespace {
constexpr int kMaxEventAmount = 600000;
}  // namespace

Acceptor::Acceptor(Poller* poller, const NetAddress& listen_address,
                   bool should_reuse_port)
    : accept_socketer_(Socketer::CreateNonblockingTcpSocket(listen_address)),
      accept_eventer_(poller, accept_socketer_.Fd()),
      is_listening_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  if (accept_socketer_.Fd() < 0) {
    int saved_errno = errno;
    LOG_ERROR("Fail to initialize for acceptor!!! errno(%d): %s", saved_errno,
              ::strerror(saved_errno));
    ::exit(-1);
  }
  accept_socketer_.SetReuseAddress(true);
  accept_socketer_.SetReusePort(should_reuse_port);
  accept_socketer_.BindAddress(listen_address);
  accept_eventer_.RegisterReadCallback([this](const TimePoint&) {
    this->SubmitAcceptOnce();
  });  // 用来触发一次提交，真正 accept 走 io_uring
  LOG_DEBUG("Acceptor init on fd(%d)", accept_socketer_.Fd());
}
Acceptor::~Acceptor() {
  LOG_DEBUG("Acceptor with fd(%d) is closing.", accept_socketer_.Fd());
  is_listening_ = false;
  ::close(idle_fd_);
}

void Acceptor::Listen() {
  is_listening_ = true;
  accept_socketer_.Listen();
  SubmitAcceptOnce();
  LOG_DEBUG("Acceptor with fd(%d) is listening.", accept_socketer_.Fd());
}

void Acceptor::DoReading() {
  SubmitAcceptOnce();
}

void Acceptor::SubmitAcceptOnce() {
  if (!is_listening_) {
    return;
  }
  auto* ctx = new AcceptContext();
  ctx->self = this;
  ctx->len = sizeof(ctx->addr);
  accept_eventer_.GetPoller()->SubmitAccept(
      accept_socketer_.Fd(),
      reinterpret_cast<struct sockaddr*>(&ctx->addr), &ctx->len, ctx,
      &Acceptor::OnAcceptComplete, static_cast<uint64_t>(accept_socketer_.Fd()),
      true /*multishot*/);
  LOG_DEBUG("Submit accept on fd(%d)", accept_socketer_.Fd());
}

void Acceptor::OnAcceptComplete(struct io_uring_cqe* cqe, Poller::IoUringOp* op) {
  auto* ctx = static_cast<AcceptContext*>(op->context);
  auto* self = ctx->self;
  int conn_fd = static_cast<int>(cqe->res);
  if (conn_fd >= 0 && conn_fd <= kMaxEventAmount) {
    NetAddress peer_address;
    peer_address.SetRawAddr(ctx->addr);
    LOG_DEBUG("Accept fd(%d) -> new fd(%d)", self->accept_socketer_.Fd(),
              conn_fd);
    if (self->NewConnectionCallback_) {
      self->NewConnectionCallback_(conn_fd, peer_address);
    } else {
      LOG_ERROR("Acceptor with fd(%d) is closing!!!", self->accept_socketer_.Fd());
      ::close(conn_fd);
    }
  } else {
    int saved_errno = (conn_fd < 0) ? -conn_fd : 0;
    if (saved_errno == 0) saved_errno = errno;
    if (saved_errno != EAGAIN && saved_errno != EWOULDBLOCK &&
        saved_errno != EINTR) {
      char errbuf[128]{};
      (void)::strerror_r(saved_errno, errbuf, sizeof(errbuf));
      LOG_ERROR(
          "Acceptor with Fd(%d) failed to accept a new TCP connection!!! "
          "errno(%d): %s",
          self->accept_socketer_.Fd(), saved_errno, errbuf);
      if (saved_errno == EMFILE) {
        ::close(self->idle_fd_);
        self->idle_fd_ = ::accept(self->accept_socketer_.Fd(), NULL, NULL);
        ::close(self->idle_fd_);
        self->idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
      }
    }
  }
  if (!(cqe->flags & IORING_CQE_F_MORE)) {
    delete ctx;
    if (self->is_listening_) {
      self->SubmitAcceptOnce();
    }
  }
}

}  // namespace taotu
