#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// How many sequence numbers are outstanding?
uint64_t TCPSender::sequence_numbers_in_flight() const
{ return next_seqno_ - acked_seqno_; }

// How many consecutive retransmissions have happened?
uint64_t TCPSender::consecutive_retransmissions() const
{ return consecutive_retransmissions_; }

void TCPSender::push( const TransmitFunction& transmit )
{
  uint64_t window = ( window_size_ == 0 ) ? 1 : window_size_;

  while ( next_seqno_ < acked_seqno_ + window ) {
    TCPSenderMessage msg;
    // 检查是不是有错误，有内鬼停止交易
    if ( input_.has_error() ) {
      msg.RST = true;
    }
    // 检查是不是需要syn
    if ( !syn_sent_ ) {
      msg.SYN = true;
      syn_sent_ = true;
    }

    msg.seqno = Wrap32::wrap( next_seqno_, isn_ );

    uint64_t window_room = window - ( next_seqno_ - acked_seqno_ );

    uint64_t len = std::min( { TCPConfig::MAX_PAYLOAD_SIZE, window_room - msg.SYN, input_.reader().bytes_buffered() } );

    // 获取payload值，并清理input_
    msg.payload = string( input_.reader().peek().substr( 0, len ) );
    input_.reader().pop( len );

    if ( input_.reader().is_finished() && !fin_sent_ && window_room > msg.sequence_length() ) {
      msg.FIN = true;
      fin_sent_ = true;
    }

    // 如果没东西可发，理应停止
    if ( msg.sequence_length() == 0 ) {
      break;
    }

    transmit( msg );

    // outstanding_ 从空变成非空应该重置 time_ = 0
    if ( outstanding_.empty() )
      time_ = 0;
    outstanding_.push( make_pair( next_seqno_ + msg.sequence_length(), msg ) );
    next_seqno_ += msg.sequence_length();
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
  // 检查是不是有错误，有内鬼停止交易
  if ( input_.has_error() ) {
    msg.RST = true;
  }
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // 检查是不是有错误，有内鬼停止交易
  if ( msg.RST ) {
    input_.set_error();
  }

  // 获取确认号
  if ( msg.ackno.has_value() ) {
    // new_ack > old_ack 重置计时器
    if ( msg.ackno->unwrap( isn_, next_seqno_ ) > next_seqno_ )
      return;
    if ( msg.ackno->unwrap( isn_, next_seqno_ ) > acked_seqno_ ) {
      time_ = 0;
      current_RTO_ = initial_RTO_ms_;
      consecutive_retransmissions_ = 0;

      acked_seqno_ = msg.ackno->unwrap( isn_, next_seqno_ );
    }
  }

  // 删除已经确认的在途序列
  while ( !outstanding_.empty() && outstanding_.front().first <= acked_seqno_ ) {
    outstanding_.pop();
  }
  // 更新窗口大小
  window_size_ = msg.window_size;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // 统计当前时间
  time_ += ms_since_last_tick;

  // 超时重传
  if ( !outstanding_.empty() && time_ >= current_RTO_ ) {
    transmit( outstanding_.front().second );
    time_ = 0; // 重传之后时间归0

    if ( window_size_ > 0 ) {
      current_RTO_ *= 2;
      consecutive_retransmissions_++;
    }
  }
}