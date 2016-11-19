#ifndef BUFFER_ITERATOR_H_
#define BUFFER_ITERATOR_H_

#include "db/version_edit.h"
#include "leveldb/slice.h"
#include "leveldb/comparator.h"
#include "db/dbformat.h"
#include "table/two_level_iterator.h"
#include <vector>
#include <assert.h>

namespace leveldb {


class BufferIterator : public Iterator {
 public:
  BufferIterator(Buffer* buffer, InternalKeyComparator* icmp)
      :buffer_(buffer), icmp_(icmp), index_((buffer->nodes).size()) { }

  virtual bool Valid() const { return index_ < (buffer_->nodes).size(); } 

  virtual Slice key() const {
    assert(Valid());
	return (buffer_->nodes)[index_].largest.Encode();
  }

  virtual void Next() { 
    assert(Valid());
    index_++;
  }

  virtual void Prev() {
    assert(Valid());
    index_ = (index_ == 0) ? (buffer_->nodes).size() : (index_ - 1);
  }

  virtual void Seek(const Slice& target){
    index_ = FindNode(icmp_, buffer_->nodes, target);
  }

  virtual void SeekToFirst() { index_ = 0; }

  virtual void SeekToLast() { index_ = (buffer_->nodes).empty() ? 0 : (buffer_->nodes).size() - 1; }


 private:

  int FindNode(const InternalKeyComparator* icmp, const std::vector<BufferNode>& nodes, const Slice& key) {
    //binary search
    uint32_t left = 0;
	uint32_t right = nodes.size();
	while (left < right) {
	  uint32_t mid = (left + right) / 2;
	  const BufferNode node = nodes[mid];
	  if (icmp->InternalKeyComparator::Compare(buffer_->largest.Encode(), key) < 0){
	    left = mid + 1;
	  } else {
	    right = mid;
	  }
	}
	return right;
  }

  Buffer* buffer_;
  uint32_t index_;
  InternalKeyComparator* icmp_;
  //Intentionally copyable
};



}


#endif
