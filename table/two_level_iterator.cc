// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "table/two_level_iterator.h"

#include "leveldb/table.h"
#include "table/block.h"
#include "db/buffer_iterator.h"
#include "table/format.h"
#include "table/iterator_wrapper.h"
#include "db/version_set.h"
#include "leveldb/slice.h"
#include <assert.h>

namespace leveldb {

namespace {

typedef Iterator* (*BlockFunction)(void*, const ReadOptions&, const Slice&);

class TwoLevelIterator: public Iterator {
 public:
  TwoLevelIterator(
    Iterator* index_iter,
    BlockFunction block_function,
    void* arg,
    const ReadOptions& options);

  virtual ~TwoLevelIterator();

  virtual void Seek(const Slice& target);
  virtual void SeekToFirst();
  virtual void SeekToLast();
  virtual void Next();
  virtual void Prev();

  virtual bool Valid() const {
    return data_iter_.Valid();
  }
  virtual Slice key() const {
    assert(Valid());
    return data_iter_.key();
  }
  virtual Slice value() const {
    assert(Valid());
    return data_iter_.value();
  }
  virtual Status status() const {
    // It'd be nice if status() returned a const Status& instead of a Status
    if (!index_iter_.status().ok()) {
      return index_iter_.status();
    } else if (data_iter_.iter() != NULL && !data_iter_.status().ok()) {
      return data_iter_.status();
    } else {
      return status_;
    }
  }

 private:
  void SaveError(const Status& s) {
    if (status_.ok() && !s.ok()) status_ = s;
  }
  void SkipEmptyDataBlocksForward();
  void SkipEmptyDataBlocksBackward();
  void SetDataIterator(Iterator* data_iter);
  void InitDataBlock();

  BlockFunction block_function_;
  void* arg_;
  const ReadOptions options_;
  Status status_;
  IteratorWrapper index_iter_;
  IteratorWrapper data_iter_; // May be NULL
  // If data_iter_ is non-NULL, then "data_block_handle_" holds the
  // "index_value" passed to block_function_ to create the data_iter_.
  std::string data_block_handle_;
};

class BufferNodeIterator : public Iterator {
 public:
  BufferNodeIterator(BufferNode* buffernode, Iterator* iterator)
      :buffernode_(buffernode), iterator_(iterator) { }

  virtual bool Valid() const;

  virtual Slice key() const;

  virtual void Next();

  virtual void Prev();

  virtual void Seek(const Slice& target);

  virtual void SeekToFirst();
  
  virtual void SeekToLast(); 

  bool SeekResult();

  virtual ~BufferNodeIterator(){
    delete iterator_;
  }


 private:
  BufferNode* buffernode_;
  Iterator* iterator_;
  bool seek_result;
};



bool BufferNodeIterator::SeekResult(){
  return seek_result; 
}


bool BufferNodeIterator::Valid() const{
  assert(iterator_->Valid());
  Slice k = iterator_->key();
  return (k.compare(buffernode_->smallest.Encode()) >= 0) 
      && (k.compare(buffernode_->largest.Encode()) <= 0);
}

Slice BufferNodeIterator::key() const {
  assert(Valid());
  return iterator_->key();
}

void BufferNodeIterator::Next() {
  assert(Valid());
  iterator_->Next();
}

void BufferNodeIterator::Prev() {
  assert(Valid());
  iterator_->Prev();
}

void BufferNodeIterator::Seek(const Slice& target) {
  seek_result = true;
  if((target.compare(buffernode_->smallest.Encode()) < 0) 
      && (target.compare(buffernode_->largest.Encode()) > 0)){
	  seek_result = false;
    return;
  }
  iterator_->Seek(target);
  if (target.compare(iterator_->key()) != 0)
    seek_result = false;
}

void BufferNodeIterator::SeekToFirst() { iterator_->Seek((buffernode_->smallest).Encode()); }
  
void BufferNodeIterator::SeekToLast() { iterator_->Seek((buffernode_->largest).Encode()); }

class BufferTwoLevelIterator : public Iterator {
 public:
  BufferTwoLevelIterator(
		  BufferIterator* index_iter,
		  BlockFunction block_function,
		  void* arg,
		  const ReadOptions)
      : block_function_(block_function),
	    arg_(arg),
	    options_(options),
		index_iter_(index_iter),
		data_iter_(NULL) {
}

  virtual bool Valid() const;

  virtual Slice key() const;

  virtual void Next();

  virtual void Prev();

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

void BufferTwoLevelIterator::InitDataBlock(){
  if (!index_iter_->Valid()) {
    delete data_iter_;
	  data_iter_ = NULL;
  } else {
    Slice handle = index_iter_->value();
	if (data_iter_ != NULL && handle.compare(data_block_handle_) == 0) {
	
	} else {
	  Iterator* iter = (*block_function_)(arg_, options_, handle);
	  data_block_handle_.assign(handle.data(), handle.size());
    assert(twolevel_iter_ != NULL);
      delete data_iter_;
      data_iter_ = new BufferNodeIterator(index_iter_.Node(), iter);
	}
  }
   
}


bool BufferTwoLevelIterator::SeekResult(){
  //if index_iter is unvalid, search do not end
  if (!index_iter_->SeekResult()) return false;
  return data_iter_->SeekResult();

}


bool BufferTwoLevelIterator::Valid() const {
  if (data_iter == NULL) return false;	
  return data_iter_->Valid();
}

Slice BufferTwoLevelIterator::key() const {
  assert(Valid());
  return data_iter_->key();
}

void BufferTwoLevelIterator::Next() {
  assert(Valid());
  data_iter_->Next();
  MaybeGotoNextNode();
}

void BufferTwoLevelIterator::Prev() {
  assert(Valid());
  data_iter_->Prev();
  MaybeGotoPrevNode();
}

void BufferTwoLevelIterator::Seek(const Slice& target) {
  //search backwards for latest key
  bool start = false;
  do {
    if (!start) {
	  index_iter_->SeekToLast();
	  start = true;
	  }  
	  else
	    index_iter_->Prev();
      index_iter_->SeekForHere(target);
	  if (!index_iter_->SeekResult()){
      delete data_iter_;
	    data_iter = NULL;
	    return;
	  }
	  InitDataBlock();
	  data_iter_->Seek(target);
  } while (!data_iter_->SeekResult())
}

void BufferTwoLevelIterator::SeekToFirst() {
  index_iter_->SeekToFirst();
  InitDataBlock();
  if (data_iter_ != NULL) data_iter_->SeekToFirst();
}
  
void BufferTwoLevelIterator::SeekToLast() { 
  index_iter_->SeekToLast();
  InitDataBlock();
  if (data_iter_ != NULL) data_iter_->SeekToLast();
}

void BufferTwoLevelIterator::MaybeGotoNextNode(){
  assert(data_iter_ != NULL)
  if (!data_iter_->Valid()) {
    index_iter_->Next();
    InitDataBlock();
    if (data_iter_ != NULL) data_iter_->SeekToFirst();
  }
}

void BufferTwoLevelIterator::MaybeGotoPrevNode(){
  assert(data_iter_ != NULL)
  if (!data_iter_->Valid()) {
    index_iter_->Prev();
    InitDataBlock();
    if (data_iter_ != NULL) data_iter_->SeekToLast();
  }
}


TwoLevelIterator::TwoLevelIterator(
    Iterator* index_iter,
    BlockFunction block_function,
    void* arg,
    const ReadOptions& options)
    : block_function_(block_function),
      arg_(arg),
      options_(options),
      index_iter_(index_iter),
      data_iter_(NULL) {
}

TwoLevelIterator::~TwoLevelIterator() {
}

void TwoLevelIterator::Seek(const Slice& target) {
  index_iter_.Seek(target);
  InitDataBlock();
  if (data_iter_.iter() != NULL) data_iter_.Seek(target);
  SkipEmptyDataBlocksForward();
}

void TwoLevelIterator::SeekToFirst() {
  index_iter_.SeekToFirst();
  InitDataBlock();
  if (data_iter_.iter() != NULL) data_iter_.SeekToFirst();
  SkipEmptyDataBlocksForward();
}

void TwoLevelIterator::SeekToLast() {
  index_iter_.SeekToLast();
  InitDataBlock();
  if (data_iter_.iter() != NULL) data_iter_.SeekToLast();
  SkipEmptyDataBlocksBackward();
}

void TwoLevelIterator::Next() {
  assert(Valid());
  data_iter_.Next();
  SkipEmptyDataBlocksForward();
}

void TwoLevelIterator::Prev() {
  assert(Valid());
  data_iter_.Prev();
  SkipEmptyDataBlocksBackward();
}


void TwoLevelIterator::SkipEmptyDataBlocksForward() {
  while (data_iter_.iter() == NULL || !data_iter_.Valid()) {
    // Move to next block
    if (!index_iter_.Valid()) {
      SetDataIterator(NULL);
      return;
    }
    index_iter_.Next();
    InitDataBlock();
    if (data_iter_.iter() != NULL) data_iter_.SeekToFirst();
  }
}

void TwoLevelIterator::SkipEmptyDataBlocksBackward() {
  while (data_iter_.iter() == NULL || !data_iter_.Valid()) {
    // Move to next block
    if (!index_iter_.Valid()) {
      SetDataIterator(NULL);
      return;
    }
    index_iter_.Prev();
    InitDataBlock();
    if (data_iter_.iter() != NULL) data_iter_.SeekToLast();
  }
}

void TwoLevelIterator::SetDataIterator(Iterator* data_iter) {
  if (data_iter_.iter() != NULL) SaveError(data_iter_.status());
  data_iter_.Set(data_iter);
}

void TwoLevelIterator::InitDataBlock() {
  if (!index_iter_.Valid()) {
    SetDataIterator(NULL);
  } else {
    Slice handle = index_iter_.value();
    if (data_iter_.iter() != NULL && handle.compare(data_block_handle_) == 0) {
      // data_iter_ is already constructed with this iterator, so
      // no need to change anything
    } else {
      Iterator* iter = (*block_function_)(arg_, options_, handle);
      data_block_handle_.assign(handle.data(), handle.size());
      SetDataIterator(iter);
    }
  }
}

}  // namespace

Iterator* NewTwoLevelIterator(
    Iterator* index_iter,
    BlockFunction block_function,
    void* arg,
    const ReadOptions& options) {
  return new TwoLevelIterator(index_iter, block_function, arg, options);
}

}  // namespace leveldb
