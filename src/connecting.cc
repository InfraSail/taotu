/**
 * @file connecting.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Connecting" which is a TCP connection
 * (status).
 * @date 2021-12-27
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "connecting.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "event_manager.h"
#include "logger.h"

namespace taotu {
namespace {
const char* StrError(int err, char* buf, size_t len) {
#if defined(_GNU_SOURCE)
  char* msg = ::strerror_r(err, buf, len);
  if (msg == nullptr || *msg == '\0') {
    ::snprintf(buf, len, "errno(%d)", err);
    return buf;
  }
  return msg;
#else
  if (::strerror_r(err, buf, len) != 0) {
    ::snprintf(buf, len, "errno(%d)", err);
  }
  if (*buf == '\0') {
    ::snprintf(buf, len, "errno(%d)", err);
  }
  return buf;
#endif
}
}  // namespace

Connecting::Connecting(EventManager* event_manager, int socket_fd,
                       const NetAddress& local_address,
                       const NetAddress& peer_address)
    : event_manager_(event_manager),
      socketer_(socket_fd),
      eventer_(event_manager->GetPoller(), socket_fd),
      local_address_(local_address),
      peer_address_(peer_address),
      state_(ConnectionState::kConnecting) {
  socketer_.SetKeepAlive(true);
  eventer_.RegisterReadCallback(
      [this](TimePoint receive_time) { this->DoReading(receive_time); });
  eventer_.RegisterWriteCallback([this] { this->DoWriting(); });
  eventer_.RegisterCloseCallback([this] { this->DoClosing(); });
  eventer_.RegisterErrorCallback([this] { this->DoWithError(); });
  LOG_DEBUG("The TCP connection with fd(%d) is being created.", socket_fd);
}
Connecting::~Connecting() {
  CancelPendingIo();
  LOG_DEBUG("The TCP connection with fd(%d) is closing.", Fd());
}

void Connecting::DoReading(TimePoint receive_time) {
  (void)receive_time;
  if (!read_in_flight_) {
    SubmitReadOnce();
  }
}

void Connecting::OnReadComplete(struct io_uring_cqe* cqe,
                                Poller::IoUringOp* op) {
  auto* ctx = static_cast<ReadContext*>(op->context);
  auto* connecting = ctx->self;
  ssize_t res = cqe->res;
  int err = res < 0 ? -res : 0;
  LOG_DEBUG("Read complete fd(%d) res(%zd) err(%d)", connecting->Fd(), res,
            err);
  bool more = (cqe->flags & IORING_CQE_F_MORE) != 0;
  bool has_buffer = (cqe->flags & IORING_CQE_F_BUFFER) != 0;
  if (ctx->multishot && has_buffer) {
    ctx->buf_id = static_cast<uint16_t>(cqe->flags >> IORING_CQE_BUFFER_SHIFT);
  }
  if (ctx->multishot && !more && !has_buffer) {
    connecting->read_in_flight_ = false;
    delete ctx;
    op->context = nullptr;
    return;
  }
  if (!more) {
    connecting->read_in_flight_ = false;
  }
  if (res > 0) {
    // Update the input buffer.
    if (ctx->multishot && has_buffer) {
      auto* buf =
          connecting->event_manager_->GetPoller()->GetBuffer(ctx->buf_id);
      if (buf) {
        connecting->input_buffer_.Append(buf, static_cast<size_t>(res));
      } else {
        LOG_WARN("buffer id out of range(%u)", ctx->buf_id);
      }
    } else {
      size_t writable = ctx->writable;
      if (static_cast<size_t>(res) <= writable) {
        connecting->input_buffer_.RefreshW(static_cast<size_t>(res));
      } else {
        connecting->input_buffer_.RefreshW(writable);
        connecting->input_buffer_.Append(ctx->extra_buffer,
                                         static_cast<size_t>(res) - writable);
      }
    }
    if (connecting->OnMessageCallback_) {
      connecting->OnMessageCallback_(*connecting, &connecting->input_buffer_,
                                     TimePoint{});
    }
    // Submit the next read continuously (re-armed after one-shot or
    // when multishot completes).
    if (!more) {
      connecting->SubmitReadOnce();
    }
  } else if (res == 0) {  // Peer closed.
    connecting->DoClosing();
  } else {  // res < 0
    if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR) {
      if (!more) {
        connecting->SubmitReadOnce();
      }
    } else if (err == ECONNRESET || err == ECONNABORTED || err == EPIPE) {
      char errbuf[128];
      const char* err_str = StrError(err, errbuf, sizeof(errbuf));
      if (err_str == nullptr || *err_str == '\0') {
        err_str = "unknown";
      }
      LOG_INFO("Peer closed/reset the connection fd(%d) err(%d - %s)",
               connecting->Fd(), err, err_str);
      connecting->DoClosing();
    } else {
      LOG_ERROR("OnReadComplete error: fd(%d) res(%zd) err(%d)",
                connecting->Fd(), res, err);
      connecting->DoWithError(err);
    }
  }
  if (!more) {
    delete ctx;
    op->context = nullptr;
  }
}

void Connecting::OnWriteComplete(struct io_uring_cqe* cqe,
                                 Poller::IoUringOp* op) {
  auto* ctx = static_cast<WriteContext*>(op->context);
  auto* connecting = ctx->self;
  connecting->write_in_flight_ = false;
  ssize_t res = cqe->res;
  int err = res < 0 ? -res : 0;
  LOG_DEBUG("Write complete fd(%d) res(%zd) err(%d)", connecting->Fd(), res,
            err);
  if (res > 0) {
    connecting->output_buffer_.Refresh(static_cast<size_t>(res));
    if (connecting->output_buffer_.GetReadableBytes() > 0) {
      connecting->SubmitWriteOnce();
    } else {
      if (connecting->pending_output_buffer_.GetReadableBytes() > 0) {
        connecting->output_buffer_.Swap(connecting->pending_output_buffer_);
        connecting->SubmitWriteOnce();
        delete ctx;
        return;
      }
      if (connecting->WriteCompleteCallback_) {
        connecting->WriteCompleteCallback_(*connecting);
      }
      if (Connecting::ConnectionState::kDisconnecting ==
              connecting->state_.load() &&
          connecting->output_buffer_.GetReadableBytes() == 0 &&
          connecting->pending_output_buffer_.GetReadableBytes() == 0) {
        connecting->socketer_.ShutdownWrite();
      }
    }
  } else {
    if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR) {
      connecting->SubmitWriteOnce();
    } else {
      LOG_ERROR("OnWriteComplete error: fd(%d) res(%zd) err(%d)",
                connecting->Fd(), res, err);
      connecting->DoWithError(err);
    }
  }
  delete ctx;
  op->context = nullptr;
}

void Connecting::SubmitReadOnce() {
  if (read_in_flight_) {
    return;
  }
  auto* ctx = new ReadContext();
  ctx->self = this;
  ctx->writable = input_buffer_.GetWritableBytes();
  ctx->iov[0].iov_base = const_cast<char*>(input_buffer_.GetWritablePosition());
  ctx->iov[0].iov_len = ctx->writable;
  ctx->iov[1].iov_base = ctx->extra_buffer;
  ctx->iov[1].iov_len = sizeof(ctx->extra_buffer);
  int iovcnt;
  if (ctx->writable == 0) {
    iovcnt = 1;
    ctx->iov[0] = ctx->iov[1];
  } else {
    iovcnt = ctx->writable < sizeof(ctx->extra_buffer) ? 2 : 1;
  }
  // ctx->key = next_io_key_++; // Deprecated: let Poller generate key
  // read_cancel_key_ = ctx->key; // Do not set yet
  read_in_flight_ = true;
#ifdef IORING_OP_RECV_MULTISHOT
  if (event_manager_->GetPoller()->BuffersRegistered()) {
    ctx->multishot = true;
    uint64_t key = event_manager_->GetPoller()->SubmitReadMultishot(
        &eventer_, Poller::kBufferGroupId, &Connecting::OnReadComplete, ctx, 0,
        [](void* ptr) { delete static_cast<ReadContext*>(ptr); });
    if (key == 0) {
      read_in_flight_ = false;
      read_cancel_key_ = 0;
      delete ctx;
      return;
    }
    ctx->key = key;
    read_cancel_key_ = key;
    return;
  }
#endif
  ctx->multishot = false;
  uint64_t key = event_manager_->GetPoller()->SubmitRead(
      &eventer_, ctx->iov.data(), iovcnt, &Connecting::OnReadComplete, ctx, 0,
      [](void* ptr) { delete static_cast<ReadContext*>(ptr); });
  if (key == 0) {
    read_in_flight_ = false;
    read_cancel_key_ = 0;
    delete ctx;
    return;
  }
  ctx->key = key;
  read_cancel_key_ = key;
}
void Connecting::DoWriting() {
  if (!write_in_flight_ && output_buffer_.GetReadableBytes() > 0) {
    SubmitWriteOnce();
  }
}
void Connecting::SubmitWriteOnce() {
  if (write_in_flight_ || output_buffer_.GetReadableBytes() == 0) {
    return;
  }
  auto* ctx = new WriteContext();
  ctx->self = this;
  ctx->to_send = output_buffer_.GetReadableBytes();
  ctx->iov.iov_base = const_cast<char*>(output_buffer_.GetReadablePosition());
  ctx->iov.iov_len = ctx->to_send;
  // ctx->key = next_io_key_++; // Deprecated
  // write_cancel_key_ = ctx->key;
  write_in_flight_ = true;
  uint64_t key = event_manager_->GetPoller()->SubmitWrite(
      &eventer_, &ctx->iov, 1, &Connecting::OnWriteComplete, ctx, 0,
      [](void* ptr) { delete static_cast<WriteContext*>(ptr); });
  if (key == 0) {
    write_in_flight_ = false;
    write_cancel_key_ = 0;
    delete ctx;
    return;
  }
  ctx->key = key;
  write_cancel_key_ = key;
}
void Connecting::DoClosing() {
  if (state_.load() != ConnectionState::kDisconnected) {
    LOG_DEBUG("Fd(%d) with state(\"%s\") is closing.", Fd(),
              GetConnectionStateInfo(state_).c_str());
    SetState(ConnectionState::kDisconnected);
    StopReadingWriting();
    CancelPendingIo();
    if (OnConnectionCallback_) {
      OnConnectionCallback_(*this);
    }
    if (CloseCallback_) {
      CloseCallback_(*this);
    }
    event_manager_->DeleteConnection(
        Fd());  // Postpone the real destroying to the end of this loop
  }
}
void Connecting::DoWithError() const { DoWithError(0); }

void Connecting::DoWithError(int err) const {
#ifdef __linux__
  int opt_val;
  auto opt_len = static_cast<socklen_t>(sizeof(opt_val));
  int saved_errno;
  if (err != 0) {
    saved_errno = err;
  } else if (::getsockopt(Fd(), SOL_SOCKET, SO_ERROR,
                          reinterpret_cast<void*>(&opt_val), &opt_len) < 0) {
    saved_errno = errno;
  } else {
    saved_errno = opt_val;
  }
#else
  int saved_errno = err != 0 ? err : errno;
#endif
  if (saved_errno == 0) {
    LOG_WARN("Fd(%d) error callback but SO_ERROR=0", Fd());
    return;
  }
  char errno_info[512];
  const char* err_str = StrError(saved_errno, errno_info, sizeof(errno_info));
  if (err_str == nullptr || *err_str == '\0') {
    err_str = "unknown";
  }
  LOG_ERROR("Fd(%d) gets an error -- errno(%d) -- %s!!!", Fd(), saved_errno,
            err_str);
}

void Connecting::OnEstablishing() {
  if (ConnectionState::kConnecting ==
      state_.load()) {  // This TCP connection can only be created once
    SetState(ConnectionState::kConnected);
    OnConnectionCallback_(*this);
    SubmitReadOnce();
  }
}

void Connecting::Send(const void* message, size_t msg_len) {
  if (ConnectionState::kDisconnected == state_.load()) {
    LOG_ERROR("Fd(%d) is disconnected, so give up sending the message!!!",
              Fd());
    return;
  }
  if (ConnectionState::kConnected == state_.load()) {
    size_t queued_len = output_buffer_.GetReadableBytes() +
                        pending_output_buffer_.GetReadableBytes();
    if (HighWaterMarkCallback_ && queued_len + msg_len >= high_water_mark_ &&
        queued_len < high_water_mark_) {
      HighWaterMarkCallback_(*this, queued_len + msg_len);
    }
    if (write_in_flight_) {
      pending_output_buffer_.Append(message, msg_len);
    } else {
      output_buffer_.Append(message, msg_len);
      SubmitWriteOnce();
    }
  }
}
void Connecting::Send(const std::string& message) {
  Send(static_cast<const void*>(message.c_str()), message.size());
}
void Connecting::Send(IoBuffer* io_buffer) {
  Send(io_buffer->GetReadablePosition(), io_buffer->GetReadableBytes());
  io_buffer->RefreshRW();
}

void Connecting::ShutDownWrite() {
  if (ConnectionState::kConnected == state_.load()) {
    SetState(ConnectionState::kDisconnecting);
    if (!eventer_
             .HasWriteEvents()) {  // If the writing event is not paid attention
                                   // to, shut down the writing end (this end)
      socketer_.ShutdownWrite();
    }
  }
}

void Connecting::ForceClose() {
  if (ConnectionState::kDisconnected != state_.load()) {
    SetState(ConnectionState::kDisconnecting);
    DoClosing();
  }
}

// FIXME: Make it be effective in the condition that the connection has been
// destroyed.
void Connecting::ForceCloseAfter(int64_t delay_microseconds) {
  if (ConnectionState::kDisconnected != state_.load()) {
    event_manager_->RunAfter(delay_microseconds,
                             [this]() { this->ForceClose(); });
  }
}

void Connecting::CancelPendingIo() {
  if (read_in_flight_) {
    if (read_cancel_key_ != 0) {
      event_manager_->GetPoller()->CancelOp(read_cancel_key_);
      read_cancel_key_ = 0;
    }
    read_in_flight_ = false;
  }
  if (write_in_flight_) {
    if (write_cancel_key_ != 0) {
      event_manager_->GetPoller()->CancelOp(write_cancel_key_);
      write_cancel_key_ = 0;
    }
    write_in_flight_ = false;
  }
}

std::string Connecting::GetConnectionStateInfo(ConnectionState state) {
  switch (state) {
    case ConnectionState::kDisconnected:
      return "Disconnected";
    case ConnectionState::kConnecting:
      return "Connecting";
    case ConnectionState::kConnected:
      return "Connected";
    case ConnectionState::kDisconnecting:
      return "Disconnecting";
  }
  return std::string{};
}

}  // namespace taotu
