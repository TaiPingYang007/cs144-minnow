#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "arp_message.hh"
#include "common.hh"
#include "helpers.hh"
#include "network_interface.hh"

inline std::string concat( const std::vector<Buffer>& buffers )
{
  std::string ret;
  for ( const auto& buffer : buffers ) {
    ret.append( std::string_view { buffer } );
  }
  return ret;
}

inline std::vector<Buffer> clone_payload( const std::vector<Buffer>& payload )
{
  std::vector<Buffer> ret;
  ret.reserve( payload.size() );
  for ( const auto& buffer : payload ) {
    ret.emplace_back( std::string { std::string_view { buffer } } );
  }
  return ret;
}

inline EthernetFrame clone( const EthernetFrame& frame )
{
  return { .header = frame.header, .payload = clone_payload( frame.payload ) };
}

inline InternetDatagram clone( const InternetDatagram& datagram )
{
  return { datagram.header, clone_payload( datagram.payload ) };
}

inline std::string summary( const EthernetFrame& frame )
{
  std::string out = frame.header.to_string() + " payload: ";
  switch ( frame.header.type ) {
    case EthernetHeader::TYPE_IPv4: {
      InternetDatagram dgram;
      if ( parse( dgram, frame.payload ) ) {
        out.append( dgram.header.to_string() + " payload=\"" + pretty_print( concat( dgram.payload ) ) + "\"" );
      } else {
        out.append( "bad IPv4 datagram" );
      }
    } break;
    case EthernetHeader::TYPE_ARP: {
      ARPMessage arp;
      if ( parse( arp, frame.payload ) ) {
        out.append( arp.to_string() );
      } else {
        out.append( "bad ARP message" );
      }
    } break;
    default:
      out.append( "unknown frame type" );
      break;
  }
  return out;
}

class FramesOut : public NetworkInterface::OutputPort
{
public:
  std::queue<EthernetFrame> frames {};
  void transmit( const NetworkInterface& n [[maybe_unused]], const EthernetFrame& x ) override
  {
    frames.push( clone( x ) );
  }

  EthernetFrame expect_frame() const
  {
    if ( frames.empty() ) {
      throw ExpectationViolation( "should have sent an Ethernet frame" );
    }
    auto& mutable_output = const_cast<decltype( frames )&>( frames ); // NOLINT(*-const-cast)
    EthernetFrame ret { std::move( mutable_output.front() ) };
    mutable_output.pop();
    return ret;
  }
};

using Output = std::shared_ptr<FramesOut>;
using InterfaceAndOutput = std::pair<NetworkInterface, Output>;

class NetworkInterfaceTestHarness : public TestHarness<InterfaceAndOutput>
{
public:
  NetworkInterfaceTestHarness( std::string test_name,
                               const EthernetAddress& ethernet_address,
                               const Address& ip_address )
    : TestHarness( move( test_name ), "eth=" + to_string( ethernet_address ) + ", ip=" + ip_address.ip(), [&] {
      Output output { std::make_shared<FramesOut>() };
      NetworkInterface iface { "test", output, ethernet_address, ip_address };
      return InterfaceAndOutput { std::move( iface ), std::move( output ) };
    }() )
  {}
};

struct SendDatagram : public Action<InterfaceAndOutput>
{
  InternetDatagram dgram;
  Address next_hop;

  std::string description() const override
  {
    return "request to send datagram (to next hop " + next_hop.ip() + "): " + dgram.header.to_string()
           + " payload=\"" + pretty_print( concat( dgram.payload ) ) + "\"";
  }

  void execute( InterfaceAndOutput& interface ) const override
  {
    interface.first.send_datagram( clone( dgram ), next_hop );
  }

  SendDatagram( const InternetDatagram& d, Address n ) : dgram( clone( d ) ), next_hop( n ) {}
};

template<class T>
bool equal( const T& t1, const T& t2 )
{
  const auto t1s = serialize( t1 );
  const auto t2s = serialize( t2 );

  return concat( t1s ) == concat( t2s );
}

struct ReceiveFrame : public Action<InterfaceAndOutput>
{
  EthernetFrame frame;
  std::optional<InternetDatagram> expected {};

  std::string description() const override { return "receive frame (" + summary( frame ) + ")"; }
  void execute( InterfaceAndOutput& interface ) const override
  {
    interface.first.recv_frame( clone( frame ) );

    auto& inbound = interface.first.datagrams_received();

    if ( not expected.has_value() ) {
      if ( inbound.empty() ) {
        return;
      }
      throw ExpectationViolation(
        "an arriving Ethernet frame was passed up the stack as an Internet datagram, but was not expected to be "
        "(did destination address match our interface?)" );
    }

    if ( inbound.empty() ) {
      throw ExpectationViolation(
        "an arriving Ethernet frame was expected to be passed up the stack as an Internet datagram, but wasn't" );
    }

    if ( not equal( inbound.front(), expected.value() ) ) {
      throw ExpectationViolation(
        std::string( "NetworkInterface::recv_frame() produced a different Internet datagram than was expected: " )
        + "actual={" + inbound.front().header.to_string() + "}" );
    }

    inbound.pop();
  }

  ReceiveFrame( const EthernetFrame& f, const InternetDatagram& e ) : frame( clone( f ) ), expected( clone( e ) ) {}

  explicit ReceiveFrame( const EthernetFrame& f ) : frame( clone( f ) ) {}
};

struct ExpectFrame : public Expectation<InterfaceAndOutput>
{
  EthernetFrame expected;

  std::string description() const override { return "frame transmitted (" + summary( expected ) + ")"; }
  void execute( const InterfaceAndOutput& interface ) const override
  {
    const EthernetFrame frame = interface.second->expect_frame();

    if ( not equal( frame, expected ) ) {
      throw ExpectationViolation( "NetworkInterface sent a different Ethernet frame than was expected: actual={"
                                  + summary( frame ) + "}" );
    }
  }

  explicit ExpectFrame( const EthernetFrame& e ) : expected( clone( e ) ) {}
};

struct ExpectNoFrame : public Expectation<InterfaceAndOutput>
{
  std::string description() const override { return "no frame transmitted"; }
  void execute( const InterfaceAndOutput& interface ) const override
  {
    if ( not interface.second->frames.empty() ) {
      throw ExpectationViolation( "NetworkInterface sent an Ethernet frame although none was expected" );
    }
  }
};

struct Tick : public Action<InterfaceAndOutput>
{
  size_t _ms;

  std::string description() const override { return to_string( _ms ) + " ms pass"; }
  void execute( InterfaceAndOutput& interface ) const override { interface.first.tick( _ms ); }

  explicit Tick( const size_t ms ) : _ms( ms ) {}
};
