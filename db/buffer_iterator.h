#ifndef BUFFER_ITERATOR_H_
#define BUFFER_ITERATOR_H_


#include <vector>
#include <assert.h>
#include <string>
#include "db/version_edit.h"
#include "db/dbformat.h"
#include "leveldb/slice.h"
#include "leveldb/iterator.h"
#include "table/two_level_iterator.h"

namespace leveldb {

using std::vector;


class BufferIterator : public Iterator {
 public:
  //when index_ == buffer->nodes.size(), then index_ is unvalid
  BufferIterator(Buffer* buffer, InternalKeyComparator icmp, vector<FileMetaData*>* flist);

  virtual bool Valid() const;

  virtual Slice key() const;
  
  virtual Slice value() const;


  FileMetaData* File() const;


  BufferNode& BNode() const;

  //never use this function
  virtual Status status() const { return Status::OK(); }


  virtual void Next();

  bool SeekResult();

  virtual void Prev();

  virtual void Seek(const Slice& target);
  
  void SeekFromHere(const Slice& target);

  //SeekToFirst do not promise iterator is valid
  virtual void SeekToFirst();

  virtual void SeekToLast();

  virtual ~BufferIterator();


 private:

  void FindLatestNode(const InternalKeyComparator icmp, const std::vector<BufferNode>& nodes, const Slice& key);
    
  
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

  std::vector<FileMetaData*>* flist_;
  Buffer* buffer_;
  uint32_t index_;
  InternalKeyComparator icmp_;
  //Intentionally copyable
};


class BufferNodeIterator : public Iterator {
 public:
  BufferNodeIterator(BufferNode* buffernode, Iterator* iterator)
      :buffernode_(buffernode), iterator_(iterator) { }

  virtual bool Valid() const;

  virtual Slice key() const;

  virtual void Next();

  //never use this function
  virtual Status status() const { return Status::OK(); }

  virtual void Prev();

  virtual void Seek(const Slice& target);

  virtual void SeekToFirst();
  
  virtual void SeekToLast(); 

  virtual Slice value() const;

  bool SeekResult();

  virtual ~BufferNodeIterator(){
    delete iterator_;
  }


 private:
  BufferNode* buffernode_;
  Iterator* iterator_;
  bool seek_result_;
};


namespace {



class BufferTwoLevelIterator : public Iterator {
 public:
  BufferTwoLevelIterator(
      BufferIterator* index_iter,
      BlockFunction block_function,
      void* arg,
      const ReadOptions& options)
      : block_function_(block_function),
      arg_(arg),
      options_(options),
    index_iter_(index_iter),
    data_iter_(NULL) {
}

  virtual bool Valid() const;

  virtual Slice key() const;

  virtual void Next();

  //never use this function
  virtual Status status() const { return Status::OK(); }

  virtual void Prev();

  virtual Slice value() const;

  virtual void Seek(const Slice& target);

  virtual void SeekToFirst();
  
  virtual void SeekToLast(); 

  bool SeekResult();

  virtual ~BufferTwoLevelIterator(){
    delete data_iter_;
    delete index_iter_;
  }


 private:

  void MaybeGotoNextNode();
  void MaybeGotoPrevNode();
  void InitDataBlock();

  BlockFunction block_function_;
  BufferIterator* index_iter_;
  BufferNodeIterator* data_iter_;
  void* arg_;
  const ReadOptions options_;
  std::string data_block_handle_;

};


} // namespace 

} // namespace leveldb


#endif
