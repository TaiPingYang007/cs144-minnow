#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // 判断是不是已经结束了，如果已经结束了就直接丢弃
  if ( output.is_closed() && eof_received_ ) {
    return;
  }

  // 如果收到 is_last_substring，记录流的结束位置（eof_index_）和是否已收到结束标志（eof_received_）
  if ( is_last_substring ) {
    eof_received_ = true;
    eof_index_ = first_index + data.size();
  }

  uint64_t next_index = output.bytes_pushed();
  // 情况A frist_index == next_index
  if ( first_index == next_index ) {
    // 计算实际可以写入的字节数
    uint64_t actual = min( data.size(), static_cast<uint64_t>( output.available_capacity() ) );
    // 生成实际可以写入的字节数的子串
    data = data.substr( 0, actual );
  }

  // 情况B frist_index < next_index
  else if ( first_index < next_index ) {
    if ( first_index + data.size() <= next_index )
      return; // 说明这个片段完全在已写窗口之前，直接丢弃

    // 说明已经有部分字节被写入了，计算剩余未写入的字节数
    uint64_t offset = next_index - first_index;
    uint64_t data_len = data.size() - offset;
    uint64_t actual = min( data_len, static_cast<uint64_t>( output.available_capacity() ) );
    data = data.substr( offset, actual );
  }

  // 情况C frist_index >= next_index
  else {
    if ( first_index >= next_index + output.available_capacity() )
      return; // 说明这个片段完全在可写窗口之外，直接丢弃
    uint64_t actual = min( data.size(),
                           static_cast<uint64_t>( next_index + output.available_capacity() )
                             - first_index ); // 计算实际可以写入的字节数（窗口末尾 - 片段起始位置）
    data = data.substr( 0, actual );
  }

  // 将重叠或相邻的部分合并到一起，写入Reassembler缓冲区
  uint64_t start = max( first_index, next_index ); // 片段起始位置
  auto it = buffer_.lower_bound( start );          // 找到第一个起始位置不小于start的片段

  // 左
  if ( it != buffer_.begin() ) // 如果it不是第一个元素，说明可能有片段在start之前，检查是否相邻或重叠
  {
    auto prev_it = std::prev( it ); // prev是it的前一个元素

    // 判断合并条件，相邻或者重叠
    if ( prev_it->first + prev_it->second.size() >= start && start + data.size() >= prev_it ->first )
    {
      // 求重叠长度
      uint64_t overlap_len = min( prev_it->first + prev_it->second.size(), start + data.size() ) - start;
    
      // 如果重叠部分为空，说明相邻但不重叠，直接合并
      if ( overlap_len == 0 ) {
        start = prev_it->first; // 更新start为prev的起始位置
        data = prev_it->second + data;
        buffer_.erase( prev_it ); // 删除prev_it，因为它已经被data完全覆盖了
      }
      // 如果重叠部分大于等于data.size()，说明data完全被prev覆盖，合并结果就是prev原串（TCP保证同编号字节一致）
      else if ( overlap_len >= data.size() ) {
        data = prev_it->second;
        start = prev_it->first;
        buffer_.erase( prev_it ); // 注销旧条目，统一出口会重新入库
      }
      // 否则prev只盖住data的开头，合并 = prev整串 + data没被盖住的尾巴
      else {
        data = prev_it->second + data.substr( overlap_len ); // 先用旧值把账算完
        start = prev_it->first;                              // 最后一步才改start
        buffer_.erase( prev_it );
      }
    }
  }

  // 右
  while ( it != buffer_.end() && it->first <= start + data.size() ) {
    // 判断合并条件，相邻或者重叠
    if ( start + data.size() >= it->first && it->first + it->second.size() >= start ) {
      // 找重叠位置和重叠长度
      // 重叠开始的位置是it->first
      uint64_t overlap_len = min( it->first + it->second.size(), start + data.size() ) - it->first;
      // 如果重叠部分为空，说明相邻但不重叠，直接合并
      if ( overlap_len == 0 ) {
        data += it->second;
      }
      // 如果重叠部分大于等于it->second.size()，说明it完全被覆盖了，直接删除it
      else if ( overlap_len >= it->second.size() ) {
        // 直接删除it，不需要更新data，因为it完全被覆盖了
      }
      // 否则说明it部分被覆盖了，更新it->second为未被覆盖的部分
      else {
        it->second = it->second.substr( overlap_len, it->second.size() - overlap_len );
        data += it->second;
      }
      it = buffer_.erase( it ); // 删除it并返回下一个迭代器
    } else
      break; // 不相邻或重叠，可直接写入
  }

  // 将本次可写入字符串写入Reassembler缓冲区
  buffer_[start] = data;

  // push统一处理
  while ( true ) {
    next_index = output.bytes_pushed();
    auto ready = buffer_.find( next_index );
    if ( ready == buffer_.end() || output.available_capacity() == 0 ) {
      break;
    }
    // 处理找到的字节
    output.push( ready->second );

    // 判断结束字符是否已push,如果已push且已收到is_last_substring，则关闭流
    if ( ready->first + ready->second.size() >= eof_index_ && eof_received_ ) {
      output.close();
    }

    buffer_.erase( ready ); // 删除已处理的片段
  }
}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t total = 0;
  for ( const auto& [key, value] : buffer_ ) {
    total += value.size(); // 只累加真实存在的字符串长度
  }

  return total;
}
