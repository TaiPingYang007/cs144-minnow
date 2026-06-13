#include "tcp_receiver.hh"
#include <algorithm>
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // 判断是否第一次收到消息，第一次收到消息时确定 ISN
  if ( message.SYN )
    isn_ = message.seqno;
  // 如果还没有确定 ISN 就收到了非 SYN 消息，直接丢弃
  if ( !isn_.has_value() )
    return;

  uint64_t checkpoint
    = inbound_stream.bytes_pushed() + 1; // checkpoint 是当前的绝对序列号，等于已经按序写入流的字节数 + 1

  uint64_t absolute_index = message.seqno.unwrap( *isn_, checkpoint ); // 计算消息的64 位 absolute seqno

  uint64_t stream_index = absolute_index + message.SYN - 1; // 计算stream_index

  reassembler.insert(
    stream_index, message.payload, message.FIN, inbound_stream ); // 将消息的 payload 插入 Reassembler
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  if ( isn_.has_value() )
    return TCPReceiverMessage {
      Wrap32::wrap( inbound_stream.bytes_pushed() + 1 + ( inbound_stream.is_closed() ? 1 : 0 ), *isn_ ),
      static_cast<uint16_t>(
        std::min( inbound_stream.available_capacity(), static_cast<uint64_t>( UINT16_MAX ) ) ) };
  else
    return TCPReceiverMessage { std::nullopt,
                                static_cast<uint16_t>( std::min( inbound_stream.available_capacity(),
                                                                 static_cast<uint64_t>( UINT16_MAX ) ) ) };
}
