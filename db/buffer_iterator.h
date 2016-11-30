#ifndef BUFFER_ITERATOR_H_
#define BUFFER_ITERATOR_H_

#include "leveldb/status.h"
#include "db/version_edit.h"
#include "leveldb/slice.h"
#include "leveldb/comparator.h"
#include "db/dbformat.h"
#include "table/two_level_iterator.h"
#include <vector>
#include <assert.h>
#include <string>

namespace leveldb {



class BufferIterator : public Iterator {
 public:
  //when index_ == buffer->nodes.size(), then index_ is unvalid
  BufferIterator(Buffer* buffer, InternalKeyComparator icmp, vector<FileMetaData*>* flist)
      :buffer_(buffer),
	  icmp_(icmp),
      flist_(flist),
	  index_((buffer->nodes).size()) { }

  virtual bool Valid() const { return index_ < (buffer_->nodes).size(); } 

  virtual Slice key() const {
    assert(Valid());
	  return (buffer_->nodes)[index_].largest.Encode();
  }
  
  virtual Slice value() const {
    assert(Valid());
    for (int i = 0;i < flist_->size();++i){
      if (buffer_[index_]->number == flist_[i]->number){
        EncodeFixed64(value_buf_, (*flist_)[i]->number);
        EncodeFixed64(value_buf_+8, (*flist_)[i]->file_size);
        return Slice(value_buf_, sizeof(value_buf_));
      }
    }
    return Slice("");
    
  }


  FileMetaData* File() const {
    assert(valid());
    for (int i = 0;i < flist_->size();++i){
      if (buffer_[index_]->number == flist_[i]->number)
        return flist_[index];
    }
    return NULL;
  }


  BufferNode* Node() const {
	  assert(Valid());
    return &((buffer_->nodes)[index_]);
  }



  virtual void Next() { 
    assert(Valid());
    index_++;
  }

  bool SeekResult(){
	  return Valid();
  }

  virtual void Prev() {
    assert(Valid());
    index_ = (index_ == 0) ? (buffer_->nodes).size() : (index_ - 1);
  }

  virtual void Seek(const Slice& target){
	  SeekToLast();  
    FindLatestNode(icmp_, buffer_->nodes, target);
    //index_ = FindNode(icmp_, buffer_->nodes, target);
  }
  
  void SeekForHere(const Slice& target){
    FindLatestNode(icmp_, buffer_->nodes, target);
  }

  //SeekToFirst do not promise iterator is valid
  virtual void SeekToFirst() {
	index_ = 0;
  }

  virtual void SeekToLast() { index_ = ((buffer_->nodes).empty()) ? 0 : (buffer_->nodes).size() - 1; }

  virtual ~BufferIterator(){
  }


 private:

  void FindLatestNode(const InternalKeyComparator icmp, const std::vector<BufferNode>& nodes, const Slice& key) {
  	for (;Valid();Prev()) {
	  const BufferNode& node = nodes[index_];	
	  if ((icmp.Compare(node.largest.Encode(), key) >= 0) 
			  && (icmp.Compare(node.smallest.Encode(), key) <= 0))	  
		  return;
	}
	  
  }

/*  
  int FindNode(const InternalKeyComparator icmp, const std::vector<BufferNode>& nodes, const Slice& key) {
    //binary search
    uint32_t left = 0;
	uint32_t right = nodes.size();
	while (left < right) {
	  uint32_t mid = (left + right) / 2;
	  const BufferNode node = nodes[mid];
	  if (icmp->InternalKeyComparator::Compare(node.largest.Encode(), key) < 0){
	    left = mid + 1;
	  } else if (icmp->InternalKeyComparator::Compare(node.smallest.Encode(), key) > 0){
	    right = mid;
	  } else {
        return mid;
	  }
	}
	return right;
  }
*/


  // Backing store for value().  Holds the file number and size.
  mutable char value_buf_[16];

  std::vector<FileMetaData*>* const flist_;
  Buffer* buffer_;
  uint32_t index_;
  InternalKeyComparator icmp_;
  //Intentionally copyable
};



}


#endif
