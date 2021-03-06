#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(operations)

add_executable(TestBPWriteReadADIOS2 TestBPWriteReadADIOS2.cpp)
target_link_libraries(TestBPWriteReadADIOS2 adios2 gtest)

add_executable(TestBPWriteReadADIOS2fstream TestBPWriteReadADIOS2fstream.cpp)
target_link_libraries(TestBPWriteReadADIOS2fstream adios2 gtest)

add_executable(TestBPWriteReadADIOS2stdio TestBPWriteReadADIOS2stdio.cpp)
target_link_libraries(TestBPWriteReadADIOS2stdio adios2 gtest)

add_executable(TestBPWriteReadAsStreamADIOS2 TestBPWriteReadAsStreamADIOS2.cpp)
target_link_libraries(TestBPWriteReadAsStreamADIOS2 adios2 gtest)

add_executable(TestBPWriteReadAsStreamADIOS2_Threads
  TestBPWriteReadAsStreamADIOS2_Threads.cpp
)
target_link_libraries(TestBPWriteReadAsStreamADIOS2_Threads adios2 gtest)

add_executable(TestBPWriteReadAttributesADIOS2
  TestBPWriteReadAttributesADIOS2.cpp
)
target_link_libraries(TestBPWriteReadAttributesADIOS2 adios2 gtest)

add_executable(TestStreamWriteReadHighLevelAPI
  TestStreamWriteReadHighLevelAPI.cpp
)
target_link_libraries(TestStreamWriteReadHighLevelAPI adios2 gtest)

add_executable(TestBPWriteFlushRead TestBPWriteFlushRead.cpp)
target_link_libraries(TestBPWriteFlushRead adios2 gtest)

add_executable(TestBPWriteMultiblockRead TestBPWriteMultiblockRead.cpp)
target_link_libraries(TestBPWriteMultiblockRead adios2 gtest)

add_executable(TestBPWriteReadMultiblock TestBPWriteReadMultiblock.cpp)
target_link_libraries(TestBPWriteReadMultiblock adios2 gtest)

add_executable(TestBPWriteReadVector TestBPWriteReadVector.cpp)
target_link_libraries(TestBPWriteReadVector adios2 gtest)

add_executable(TestBPWriteReadAttributesMultirank TestBPWriteReadAttributesMultirank.cpp)
target_link_libraries(TestBPWriteReadAttributesMultirank adios2 gtest)

add_executable(TestBPLargeMetadata TestBPLargeMetadata.cpp)
target_link_libraries(TestBPLargeMetadata adios2 gtest)

add_executable(TestBPWriteMemorySelectionRead TestBPWriteMemorySelectionRead.cpp)
target_link_libraries(TestBPWriteMemorySelectionRead adios2 gtest)

add_executable(TestBPWriteReadLocalVariables TestBPWriteReadLocalVariables.cpp)
target_link_libraries(TestBPWriteReadLocalVariables adios2 gtest)

if(ADIOS2_HAVE_MPI)

  target_link_libraries(TestBPWriteReadADIOS2 MPI::MPI_C)
  target_link_libraries(TestBPWriteReadADIOS2fstream MPI::MPI_C)
  target_link_libraries(TestBPWriteReadADIOS2stdio MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAsStreamADIOS2 MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAsStreamADIOS2_Threads MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAttributesADIOS2 MPI::MPI_C)
  target_link_libraries(TestStreamWriteReadHighLevelAPI MPI::MPI_C)
  target_link_libraries(TestBPWriteFlushRead MPI::MPI_C)
  target_link_libraries(TestBPWriteMultiblockRead MPI::MPI_C)
  target_link_libraries(TestBPWriteReadMultiblock MPI::MPI_C)
  target_link_libraries(TestBPWriteReadVector MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAttributesMultirank MPI::MPI_C)
  target_link_libraries(TestBPLargeMetadata MPI::MPI_C)
  target_link_libraries(TestBPWriteMemorySelectionRead MPI::MPI_C)
  target_link_libraries(TestBPWriteReadLocalVariables MPI::MPI_C)
  
  add_executable(TestBPWriteAggregateRead TestBPWriteAggregateRead.cpp)
  target_link_libraries(TestBPWriteAggregateRead
    adios2 gtest_interface MPI::MPI_C
  )
  
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
  gtest_add_tests(TARGET TestBPWriteAggregateRead ${extra_test_args})
  
endif()

gtest_add_tests(TARGET TestBPWriteReadADIOS2 ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadADIOS2fstream ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadADIOS2stdio ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadAsStreamADIOS2 ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadAsStreamADIOS2_Threads ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadAttributesADIOS2 ${extra_test_args})
gtest_add_tests(TARGET TestStreamWriteReadHighLevelAPI ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteFlushRead ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteMultiblockRead ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadMultiblock ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadVector ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadAttributesMultirank ${extra_test_args})
gtest_add_tests(TARGET TestBPLargeMetadata ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteMemorySelectionRead ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadLocalVariables ${extra_test_args})
