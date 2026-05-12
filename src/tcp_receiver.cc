#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  (void)message;
  (void)reassembler;
  (void)inbound_stream;
  // Your code here.
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  (void)inbound_stream;
  return {}; // Your code here.
}
