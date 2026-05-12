#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  uint64_t actual = min( static_cast<uint64_t>( data.size() ), available_capacity() );
  buffer_ += data.substr( 0, actual );
  bytes_pushed_ += actual;
}

void Writer::close()
{
  closed_ = true; // Your code here.
}

bool Writer::is_closed() const
{
  return closed_; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size(); // Your code here.
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_; // Your code here.
}

string_view Reader::peek() const
{
  return buffer_; // Your code here.
}

void Reader::pop( uint64_t len )
{
  // Your code here
  uint64_t actual = min( len, static_cast<uint64_t>( buffer_.size() ) );
  buffer_.erase( 0, actual );
  bytes_popped_ += actual;
}

bool Reader::is_finished() const
{
  return closed_ && buffer_.empty(); // Your code here.
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size(); // Your code here.
}

uint64_t Reader::bytes_popped() const
{
  return bytes_popped_; // Your code here.
}

