#include <iostream>
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

class BufferIteratorTest { };

TEST(BufferIteratorTest, BIteratorEmpty) {
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

TEST(BufferIteratorTest, BIteratorInsertAndLookUp){
  Buffer buffer;
  InternalKeyComparator icmp(BytewiseComparator());
  const int N = 3000;
  const int R = 5000;
  SequenceNumber s = 1;
  ValueType t = kTypeValue;
  BufferNode bnode;
  for (int i =1000;i < N;i+=3){
	//insert N buffernode into buffer
	bnode.size = 100;
	bnode.number = 1;
	char tmp[5];
    snprintf(tmp, sizeof(tmp), "%d", i);
	InternalKey smallest(Slice(tmp), s, t);
    snprintf(tmp, sizeof(tmp), "%d", i + 2);
	InternalKey largest(Slice(tmp), s, t);
	bnode.smallest = smallest;
	bnode.largest = largest;
	ASSERT_TRUE(icmp.Compare(bnode.smallest.Encode(), bnode.largest.Encode()) <= 0);
	buffer.nodes.push_back(bnode);
  }
  //iterator need buffer to be sorted
  
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

class BufferNodeIteratorTest { };

/*
 *
TEST(BufferNodeIteratorTest, BNIteratorEmpty){

}

TEST(BufferNodeIteratorTest, BNIteratorInsertAndLoopUp){

}

*/

}


int main(int argc, char** argv){
  return leveldb::test::RunAllTests();
}
	


	
