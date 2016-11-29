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

class Version::LevelFileNumIterator;


class BufferIterator : public Iterator {
 public:
  //when index_ == buffer->nodes.size(), then index_ is unvalid
  BufferIterator(Buffer* buffer, InternalKeyComparator* icmp, Version::LevelFileNumIterator* filenum_iter)
      :buffer_(buffer),
	  icmp_(icmp),
      filenum_iter_(filenum_iter),
	  index_((buffer->nodes).size()) { }

  virtual bool Valid() const { return index_ < (buffer_->nodes).size(); } 

  virtual Slice key() const {
    assert(Valid());
	return (buffer_->nodes)[index_].largest.Encode();
  }
  
  //never use this function
  virtual Status status() const { return Status::OK(); }


  
  //never use this function 
  virtual Slice value() const { return Slice(""); }

  BufferNode& Node() const {
	assert(Valid());
    return (buffer_->nodes)[index_];
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
    delete file_iter_;
  }

 Version::LevelFileNumIterator* GetFileIter() {
   return filenum_iter_;
 }

 private:

  void FindLatestNode(const InternalKeyComparator* icmp, const std::vector<BufferNode>& nodes, const Slice& key) {
  	for (;Valid();Prev()) {
	  const BufferNode& node = nodes[index_];	
	  if ((icmp->Compare(node.largest.Encode(), key) >= 0) 
			  && (icmp->Compare(node.smallest.Encode(), key) <= 0))	  
		  return;
	}
	  
  }

/*  
  int FindNode(const InternalKeyComparator* icmp, const std::vector<BufferNode>& nodes, const Slice& key) {
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
  LevelFileNumIterator filenum_iter_;
  Buffer* buffer_;
  uint32_t index_;
  InternalKeyComparator* icmp_;
  //Intentionally copyable
};



}


#endif
