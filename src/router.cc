#include "router.hh"
#include "debug.hh"

#include <iostream>
#include <map>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  routing_table_.push_back( { route_prefix, prefix_length, next_hop, interface_num } );

  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // 获取目的IP + 检查TTL（如果为0就丢弃） + 查路由表（获取下一跳IP） + TTL-1 + 发送
  for ( auto& interface_ : interfaces_ ) {
    // 获取目的IP
    while ( !interface_->datagrams_received().empty() ) {
      uint32_t dst_ip = interface_->datagrams_received().front().header.dst;

      // 检查TTL（如果<= 1就丢弃）
      if ( interface_->datagrams_received().front().header.ttl <= 1 ) {
        interface_->datagrams_received().pop();
        continue;
      }

      // 查路由表
      const RouteEntry* best_route = nullptr; // 目前最优,初始没有
      for ( auto& route : routing_table_ ) {
        uint32_t mask = ( route.prefix_length == 0 ) ? 0u : ( ~0u << ( 32 - route.prefix_length ) ); // 创造掩码
        // 检查是否匹配
        if ( ( route.route_prefix & mask ) == ( dst_ip & mask ) )
          if ( best_route == nullptr || route.prefix_length > best_route->prefix_length ) {
            best_route = &route;
          }
      }
      // 找到route_map中prefix_length最大的一条路由并发送，如果没有就丢弃
      if ( best_route != nullptr ) {
        interface_->datagrams_received().front().header.ttl--;
        InternetDatagram dgram = interface_->datagrams_received().front();
        dgram.header.compute_checksum();
        Address actual_next_hop
          = best_route->next_hop.has_value()
              ? best_route->next_hop.value()
              : Address::from_ipv4_numeric( dgram.header.dst ); // 如果为空，就用包自己的目的IP构造Addres

        interfaces_[best_route->interface_num]->send_datagram( dgram, actual_next_hop );
      }
      interface_->datagrams_received().pop();
    }
  }
}