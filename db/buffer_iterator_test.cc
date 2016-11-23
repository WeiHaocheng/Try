#include "db/buffer_iterator.h"
#include <string>
#include "util/testarness.h"
#include <vector>
#include "db/verion_edit.h"
#include "db/dbformat.h"

namespace leveldb{

TEST(BufferIteratorTest, Empty) {
  Buffer buffer;
  InternalKey icmp(BytewiseComparator());	
  BufferIterator biter(&buffer, icmp);	
}

	


	
}
