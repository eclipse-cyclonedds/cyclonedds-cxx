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

CU_Test(idl_ostream, write_transfer_buffer)
{
  idl_ostream_t* ostr = create_idl_ostream(NULL),
               * ostr2 = create_idl_ostream(NULL);

  const size_t buffergrowsby = (IDL_OSTREAM_BUFFER_INCR + 1);
  char* buffer = malloc(buffergrowsby);
  for (int i = 0; i < IDL_OSTREAM_BUFFER_INCR; i++)
    snprintf(buffer + i, buffergrowsby, "%d", i % 10);


  const size_t iterations = 5;
  const size_t transfers = 5;
  for (size_t transfer = 0; transfer < transfers; transfer++)
  {
    for (size_t iteration = 0; iteration < iterations; iteration++)
    {
      for (size_t i = 0; i < IDL_OSTREAM_BUFFER_INCR; i++)
      {
        format_ostream(ostr, "%d", i % 10);
        CU_ASSERT_EQUAL(get_ostream_buffer_position(ostr), iteration* IDL_OSTREAM_BUFFER_INCR+i+1);
      }

      if (transfer == 0)
      {
        if (iteration == 0)
        {
          CU_ASSERT_EQUAL(get_ostream_buffer_size(ostr), 2 * buffergrowsby - 1);
        }
        else
        {
          CU_ASSERT_EQUAL(get_ostream_buffer_size(ostr), (iteration + 1) * buffergrowsby - 1);
        }
      }
      else
      {
        CU_ASSERT_EQUAL(get_ostream_buffer_size(ostr), iterations * buffergrowsby - 1);
      }

      for (size_t i = 0; i <= iteration; i++)
        CU_ASSERT_EQUAL(0, memcmp(get_ostream_buffer(ostr) + IDL_OSTREAM_BUFFER_INCR * i, buffer, IDL_OSTREAM_BUFFER_INCR));
    }

    transfer_ostream_buffer(ostr, ostr2);
    CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(ostr));

    for (size_t i = 0; i < transfer * iterations; i++)
      CU_ASSERT_EQUAL(0, memcmp(get_ostream_buffer(ostr2) + i * IDL_OSTREAM_BUFFER_INCR, buffer, IDL_OSTREAM_BUFFER_INCR));
  }

  destruct_idl_ostream(ostr);
  destruct_idl_ostream(ostr2);
  free(buffer);
}
