#include <iostream>
#include <stdexcept>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name ), port_( move( port ) ), ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  if ( not port_ ) {
    throw runtime_error( "OutputPort: returned null pointer" );
  }

  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( InternetDatagram dgram, const Address& next_hop )
{
  // Address → uint32，当查表 key
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();

  // 情况1：缓存里有下一跳的ip对应的mac地址
  if ( arp_table_.contains( next_hop_ip ) ) {
    EthernetFrame frame;
    frame.header.src = ethernet_address_;                  // 源 MAC
    frame.header.dst = arp_table_.at( next_hop_ip ).first; // 目的 MAC
    frame.header.type = EthernetHeader::TYPE_IPv4;         // 面单类型 = IPv4
    frame.payload = serialize( dgram );
    transmit( frame );
    return;
  }

  // 情况2：缓存里没有下一跳的ip对应的mac地址————不认识 MAC → 数据报排队 + 广播 ARP 去问
  // 可能出现多个数据包需要一个next_hop_ip的mac地址的情况，所以先检查是不是已经发出去ARP广播去问了，如果是只需要把数据报存入等待区就不必重复问了
  const bool already_asked = pending_datagrams_.contains( next_hop_ip );

  // 如果ARP请求已经发出就存数据报，如果还没有发出就存数据报+时间
  pending_datagrams_[next_hop_ip].pending_datagram_.push_back( dgram );
  if ( already_asked )
    return;
  pending_datagrams_[next_hop_ip].time_ = current_time_;

  // 属于情况2并且没有发出ARP请求
  ARPMessage arp; // 造 ARP 表（= make_arp 内部那几行）
  arp.opcode = ARPMessage::OPCODE_REQUEST;
  arp.sender_ethernet_address = ethernet_address_;    // 发问人 MAC = 我
  arp.sender_ip_address = ip_address_.ipv4_numeric(); // 发问人 IP = 我
  arp.target_ip_address = next_hop_ip;                // 要问的 IP = 下一跳
  // target_ethernet_address 留默认空 —— 不知道才问

  EthernetFrame frame;
  frame.header.src = ethernet_address_;
  frame.header.dst = ETHERNET_BROADCAST; // 目的 MAC = 广播
  frame.header.type = EthernetHeader::TYPE_ARP;
  frame.payload = serialize( arp );
  transmit( frame );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  // 接收帧，首先判断是不是发给自己的或者是不是ARP请求，如果不是理应丢弃
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST )
    return;

  // 再判断是什么消息，ARP还是IPv4？
  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    // ARP
    ARPMessage acked_arp;
    parse( acked_arp, frame.payload ); // 还原ARP请求

    arp_table_[acked_arp.sender_ip_address]
      = make_pair( acked_arp.sender_ethernet_address, current_time_ ); // 接收到ARP消息就存入ARP缓存
    // 如果是需要的ARP请求回复了对应的下一跳mac，应该发送+删除缓存
    if ( pending_datagrams_.contains( acked_arp.sender_ip_address ) ) {
      for ( auto& pending_datagram_ : pending_datagrams_[acked_arp.sender_ip_address].pending_datagram_ ) {
        EthernetFrame pending_frame;
        pending_frame.header.src = ethernet_address_;
        pending_frame.header.dst = acked_arp.sender_ethernet_address;
        pending_frame.header.type = EthernetHeader::TYPE_IPv4;
        pending_frame.payload = serialize( pending_datagram_ );
        transmit( pending_frame );
      }
      pending_datagrams_.erase( acked_arp.sender_ip_address );
      // 如果这次就是答复ARP请求，那么可以结束了
      if ( acked_arp.opcode == ARPMessage::OPCODE_REPLY )
        return;
    }

    // ARP请求中判断next_hop_ip 是不是自己，是理应返回mac，不是应该丢弃
    if ( acked_arp.target_ip_address == ip_address_.ipv4_numeric()
         && acked_arp.opcode == ARPMessage::OPCODE_REQUEST ) {
      // next_hop_ip 是自己
      ARPMessage arp;
      arp.opcode = ARPMessage::OPCODE_REPLY;
      arp.sender_ethernet_address = ethernet_address_;
      arp.sender_ip_address = ip_address_.ipv4_numeric();
      arp.target_ethernet_address = acked_arp.sender_ethernet_address;
      arp.target_ip_address = acked_arp.sender_ip_address;

      EthernetFrame reply_frame;
      reply_frame.header.type = EthernetHeader::TYPE_ARP;
      reply_frame.header.src = ethernet_address_;
      reply_frame.header.dst = acked_arp.sender_ethernet_address;
      reply_frame.payload = serialize( arp );
      transmit( reply_frame );
    } else {
      return;
    }
  } else {
    // IPv4请求
    InternetDatagram dgram;
    parse( dgram, frame.payload ); // 还原数据报
    datagrams_received_.push( dgram );
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  current_time_ += ms_since_last_tick;
  // pending：5 秒过期
  std::erase_if( pending_datagrams_,
                 [&]( const auto& entry ) { return current_time_ - entry.second.time_ > 5000; } );
  // arp_table_：30 秒过期（上一条给过）
  std::erase_if( arp_table_, [&]( const auto& entry ) { return current_time_ - entry.second.second > 30000; } );
}
