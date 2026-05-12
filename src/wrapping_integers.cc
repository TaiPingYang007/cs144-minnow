#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  (void)n;
  (void)zero_point;
  return Wrap32 { 0 }; // Your code here.
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  (void)zero_point;
  (void)checkpoint;
  return {}; // Your code here.
}
