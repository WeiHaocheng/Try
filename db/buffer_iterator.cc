#include "buffer_iterator.h"




namespace leveldb {

  //when index_ == buffer->nodes.size(), then index_ is unvalid
BufferIterator::BufferIterator(Buffer* buffer, InternalKeyComparator icmp, vector<FileMetaData*>* flist)
    :buffer_(buffer),
  icmp_(icmp),
    flist_(flist),
  index_((buffer->nodes).size()) { }

bool BufferIterator::Valid() const { return index_ < (buffer_->nodes).size(); } 

Slice BufferIterator::key() const {
  assert(Valid());
  return (buffer_->nodes)[index_].largest.Encode();
}

Slice BufferIterator::value() const {
  assert(Valid());
  for (int i = 0;i < flist_->size();++i){
    uint32_t num = buffer_->nodes[index_].number;
    if (num == (*flist_)[i]->number){
      EncodeFixed64(value_buf_, (*flist_)[i]->number);
      EncodeFixed64(value_buf_+8, (*flist_)[i]->file_size);
      return Slice(value_buf_, sizeof(value_buf_));
    }
  }
  return Slice("");
  
}


FileMetaData* BufferIterator::File() const {
  assert(valid());
  uint32_t num = buffer_->nodes[index_].number;
  for (int i = 0;i < flist_->size();++i){
    if (num == (*flist_)[i]->number)
      return (*flist_)[i];
  }
  return NULL;
}


BufferNode& BufferIterator::BNode() const {
  assert(Valid());
  return ((buffer_->nodes)[index_]);
}



void BufferIterator::Next() { 
  assert(Valid());
  index_++;
}

bool BufferIterator::SeekResult(){
  return Valid();
}

void BufferIterator::Prev() {
  assert(Valid());
  index_ = (index_ == 0) ? (buffer_->nodes).size() : (index_ - 1);
}

void BufferIterator::Seek(const Slice& target){
  SeekToLast();  
  FindLatestNode(icmp_, buffer_->nodes, target);
  //index_ = FindNode(icmp_, buffer_->nodes, target);
}

void BufferIterator::SeekFromHere(const Slice& target){
  FindLatestNode(icmp_, buffer_->nodes, target);
}

//SeekToFirst do not promise iterator is valid
void BufferIterator::SeekToFirst() {
  index_ = 0;
}

void BufferIterator::SeekToLast() { index_ = ((buffer_->nodes).empty()) ? 0 : (buffer_->nodes).size() - 1; }

BufferIterator::~BufferIterator(){
}


void BufferIterator::FindLatestNode(const InternalKeyComparator icmp, const std::vector<BufferNode>& nodes, const Slice& key) {
  for (;Valid();Prev()) {
    const BufferNode& node = nodes[index_]; 
    if ((icmp.Compare(node.largest.Encode(), key) >= 0) 
        && (icmp.Compare(node.smallest.Encode(), key) <= 0))    
      return;
  }


}



bool BufferNodeIterator::SeekResult(){
  return seek_result_; 
}


bool BufferNodeIterator::Valid() const{
  if (!iterator_->Valid()) return false; 
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
  seek_result_ = true;
  if((target.compare(buffernode_->smallest.Encode()) < 0) 
      && (target.compare(buffernode_->largest.Encode()) > 0)){
    seek_result_ = false;
    return;
  }
  iterator_->Seek(target);
  if (target.compare(iterator_->key()) != 0)
    seek_result_ = false;
}

void BufferNodeIterator::SeekToFirst() { iterator_->Seek((buffernode_->smallest).Encode()); }
  
void BufferNodeIterator::SeekToLast() { iterator_->Seek((buffernode_->largest).Encode()); }


Slice BufferNodeIterator::value() const {
  return iterator_->value();
}


namespace {


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
    delete data_iter_;
    BufferNode& node = index_iter_->BNode();
    data_iter_ = new BufferNodeIterator(&node, iter);
  }
  }
    
}


bool BufferTwoLevelIterator::SeekResult(){
  //if index_iter is unvalid, search do not end
  if (!index_iter_->SeekResult()) return false;
  return data_iter_->SeekResult();

}


bool BufferTwoLevelIterator::Valid() const {
  if (data_iter_ == NULL) return false;  
  return data_iter_->Valid();
}

Slice BufferTwoLevelIterator::key() const {
  assert(Valid());
  return data_iter_->key();
}

Slice BufferTwoLevelIterator::value() const {
  assert(Valid());
  return data_iter_->value();
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
      index_iter_->SeekFromHere(target);
    if (!index_iter_->SeekResult()){
      delete data_iter_;
      data_iter_ = NULL;
      return;
    }
    InitDataBlock();
    data_iter_->Seek(target);
  } while (!data_iter_->SeekResult());
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
  assert(data_iter_ != NULL);
  if (!data_iter_->Valid()) {
    index_iter_->Next();
    InitDataBlock();
    if (data_iter_ != NULL) data_iter_->SeekToFirst();
  }
}

void BufferTwoLevelIterator::MaybeGotoPrevNode(){
  assert(data_iter_ != NULL);
  if (!data_iter_->Valid()) {
    index_iter_->Prev();
    InitDataBlock();
    if (data_iter_ != NULL) data_iter_->SeekToLast();
  }
}


} // namespace


} // namespace leveldb


