#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "idlcxx/idl_ostream.h"

#include "CUnit/Test.h"

CU_Init(idl_ostream)
{
  return 0;
}

CU_Clean(idl_ostream)
{
  return 0;
}

CU_Test(idl_ostream, creation_destruction)
{
  //!!!WARNING!!! need to use a real file here? to prevent undefined behaviour when the ostream might
  //do something on destruction with the file pointer (writing to it or something)
  FILE* f = (FILE*)0x12345678;
  idl_ostream_t* ostr = create_idl_ostream(f);

  CU_ASSERT_PTR_NOT_NULL_FATAL(ostr);

  CU_ASSERT_PTR_NOT_NULL(get_ostream_buffer(ostr));

  CU_ASSERT_EQUAL(get_ostream_buffer_size(ostr), IDL_OSTREAM_BUFFER_INCR);

  CU_ASSERT_EQUAL(get_ostream_buffer_position(ostr), 0);

  CU_ASSERT_PTR_EQUAL(get_ostream_file(ostr),f);

  destruct_idl_ostream(ostr);
}

#if 0
CU__Test(idl_ostream, write_transfer_buffer)
{
  idl_ostream_t* ostr = create_idl_ostream(NULL),
               * ostr2 = create_idl_ostream(NULL);

  char buffer[IDL_OSTREAM_BUFFER_INCR + 1];
  for (int i = 0; i < IDL_OSTREAM_BUFFER_INCR; i++)
    snprintf(buffer + i, IDL_OSTREAM_BUFFER_INCR + 1, "%d", i % 10);

  const int iterations = 5;
  const int transfers = 5;
  for (int transfer = 0; transfer < transfers; transfer++)
  {
    for (int iteration = 0; iteration < iterations; iteration++)
    {
      for (int i = 0; i < IDL_OSTREAM_BUFFER_INCR; i++)
      {
        format_ostream(ostr, "%d", i % 10);
        CU_ASSERT_EQUAL(get_ostream_buffer_position(ostr), iteration* IDL_OSTREAM_BUFFER_INCR+i+1);
      }

      if (transfer == 0)
      {
        CU_ASSERT_EQUAL(get_ostream_buffer_size(ostr), (iteration + 2) * IDL_OSTREAM_BUFFER_INCR);
      }
      else
      {
        CU_ASSERT_EQUAL(get_ostream_buffer_size(ostr), (iterations + 1) * IDL_OSTREAM_BUFFER_INCR);
      }

      for (int i = 0; i <= iteration; i++)
        CU_ASSERT_EQUAL(0, memcmp(get_ostream_buffer(ostr) + IDL_OSTREAM_BUFFER_INCR * i, buffer, IDL_OSTREAM_BUFFER_INCR));
    }

    transfer_ostream_buffer(ostr, ostr2);
    CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(ostr));

    for (int i = 0; i < transfer * iterations; i++)
      CU_ASSERT_EQUAL(0, memcmp(get_ostream_buffer(ostr2) + i * IDL_OSTREAM_BUFFER_INCR, buffer, IDL_OSTREAM_BUFFER_INCR));
  }

  destruct_idl_ostream(ostr);
  destruct_idl_ostream(ostr2);
}
#endif
