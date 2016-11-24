#include "db/buffer_iterator.h"
#include <string>
#include "util/testharness.h"
#include <vector>
#include "db/version_edit.h"
#include "util/hash.h"
#include "util/random.h"
#include "db/dbformat.h"
#include <time.h>
#include <stdlib.h>
#include <algorithm>

namespace leveldb{

bool nodecompare(const BufferNode &a, const BufferNode &b){
  InternalKeyComparator icmp(BytewiseComparator());
  return icmp.Compare(a.largest.Encode(), b.largest.Encode()) < 0;
  
}

TEST(BufferIteratorTest, Empty) {
  //empty buffer
  Buffer buffer;
  InternalKeyComparator icmp(BytewiseComparator());	
  BufferIterator bIter(&buffer, &icmp);	
  ASSERT_TRUE(!bIter.Valid());
  bIter.SeekToFirst();
  ASSERT_TRUE(!bIter.Valid());
  bIter.SeekToLast();
  ASSERT_TRUE(!bIter.Valid());
  bIter.Seek(Slice("target"));
  ASSERT_TRUE(!bIter.Valid());
}

TEST(BufferIteratorTest, InsertAndLookUp){
  Buffer buffer;
  InternalKeyComparator icmp(BytewiseComparator());
  srand((unsigned)time(NULL));
  const int N = 1000;
  const int R = 5000;
  SequenceNumber s = 1;
  ValueType t = 0x1;
  BufferNode bnode;
  for (int i =0;i < N;++i){
	//insert N buffernode into buffer
	bnode.size = 100;
	bnode.number = 1;
	char tmp[5];
    itoa(rand() % R, tmp, 10);
	InternalKey smallest(Slice(tmp), s, t);
    itoa(rand() % R, tmp, 10);
	InternalKey largest(Slice(tmp), s, t);
	if (icmp.InternalKeyComparator::Compare(smallest.Encode(), largest.Encode()) > 0){
	  InternalKey tmpkey = largest;
	  smallest = tmpkey;
	  largest = smallest;
	}
	bnode.smallest = smallest;
	bnode.largest = largest;
  }
  //iterator need buffer to be sorted
  std::sort(buffer.nodes.begin(), buffer.nodes.end(), nodecompare);
  
  BufferIterator bIter(&buffer, &icmp);
  ASSERT_TRUE(!bIter.Valid());
  
  bIter.SeekToFirst();
  ASSERT_TRUE(bIter.Valid());
  
  bIter.SeekToLast();
  ASSERT_TRUE(bIter.Valid());

  bIter.Seek(bnode.largest.Encode());
  ASSERT_TRUE(bIter.SeekResult());
  ASSERT_TRUE(icmp.Compare(bIter.key(), bnode.largest.Encode()) == 0);


}

TEST(BufferNodeIterator, Empty){

}

TEST(BufferNodeIterator, InsertAndLoopUp){

}

int main(int argc, char** argv){
  return leveldb::test::RunAllTests();
}
	


	
}
