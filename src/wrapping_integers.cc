#include "wrapping_integers.hh"

using namespace std;

// wrap:   把 64 位 absolute seqno 翻译成 32 位 wrapping seqno
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { zero_point + static_cast<uint32_t>( n ) };
}

// unwrap: 把 32 位 wrapping seqno 翻译成 64 位 absolute seqno
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // zero_point.raw_value_:ISN, raw_value_:当前的 wrapping seqno，checkpoint:当前的绝对序列号
  uint64_t ans;
  uint64_t offset = raw_value_ - zero_point.raw_value_; // 减去ISN，得到（absolute）32 位 wrapping seqno

  // 通过checkpoint辅助得到absolute index，checkpoint是一个绝对序列号，表示当前的进度位置
  uint64_t this_one;
  uint64_t next_one;
  uint64_t last_one;

  // checkpoint 大于 offset ：同、下
  // checkpoint 小于 offset ：同、上
  // 等于：同

  // 计算现在的checkpoint可回绕次数
  this_one = checkpoint / ( 1ULL << 32 );
  next_one = this_one + 1;
  last_one = this_one - 1;

  if ( checkpoint - this_one * ( 1ULL << 32 ) >= offset && this_one != 0 ) {
    // checkpoint 大于 offset ：同、下
    uint64_t diff1 = checkpoint - ( this_one * ( 1ULL << 32 ) + offset );
    uint64_t diff2 = offset + next_one * ( 1ULL << 32 ) - checkpoint;
    if ( diff1 <= diff2 ) {
      ans = this_one * ( 1ULL << 32 ) + offset;
    } else {
      ans = next_one * ( 1ULL << 32 ) + offset;
    }
  }

  if ( checkpoint - this_one * ( 1ULL << 32 ) < offset && this_one != 0 ) {
    // checkpoint 小于 offset ：同、上
    uint64_t diff1 = offset - checkpoint + this_one * ( 1ULL << 32 );
    uint64_t diff2 = checkpoint - ( last_one * ( 1ULL << 32 ) + offset );
    if ( diff1 <= diff2 ) {
      ans = this_one * ( 1ULL << 32 ) + offset;
    } else {
      ans = last_one * ( 1ULL << 32 ) + offset;
    }
  }

  if ( this_one == 0 ) {
    uint64_t diff1 = checkpoint > offset ? checkpoint - offset : offset - checkpoint;
    uint64_t diff2 = offset + next_one * ( 1ULL << 32 ) - checkpoint;
    if ( diff1 <= diff2 ) {
      ans = this_one * ( 1ULL << 32 ) + offset;
    } else {
      ans = next_one * ( 1ULL << 32 ) + offset;
    }
  }

  return ans;
}
