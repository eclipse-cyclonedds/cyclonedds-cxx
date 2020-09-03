#include "HelloWorldData.h"

namespace HelloWorldData
{

  size_t write_struct(const Msg &obj, void *data, size_t position)
  {
    size_t alignmentbytes =  (4 - position&0x3)&0x3;  //alignment
    memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0
    position += alignmentbytes;  //moving position indicator
    *((int32_t*)((char*)data+position)) = obj.userID();  //writing bytes for member: userID
    position += 4;  //moving position indicator
    uint32_t sequenceentries = obj.message().size()+1;  //number of entries in the sequence
    *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: message().size
    position += 4;  //moving position indicator
    memcpy((char*)data+position,obj.message().data(),sequenceentries*1);  //contents for message
    position += sequenceentries*1;  //moving position indicator
    return position;
  }

  size_t write_size(const Msg &obj, size_t offset)
  {
    size_t position = offset;
    position += (4 - position&0x3)&0x3;  //alignment
    position += 4;  //bytes for member: userID
    position += 4;  //bytes for member: message().size
    position += (obj.message().size()+1)*1;  //entries of sequence
    return position-offset;
  }

  size_t read_struct(Msg &obj, const void *data, size_t position)
  {
    position += (4 - position&0x3)&0x3;  //alignment
    obj.userID(*((int32_t*)((char*)data+position)));  //reading bytes for member: userID
    position += 4;  //moving position indicator
    uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence
    position += 4;  //moving position indicator
    obj.message().assign((char*)((char*)data+position),(char*)((char*)data+position)+sequenceentries);  //putting data into container
    position += sequenceentries*1;  //moving position indicator
    return position;
  }

} //end namespace HelloWorldData
