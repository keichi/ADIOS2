#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# BPLS
add_executable(bpls ./bpls/bpls.cpp)
target_link_libraries(bpls adios2 adios2sys_interface)
install(TARGETS bpls EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/bpls/bpls.cmake.gen.in
  ${PROJECT_BINARY_DIR}/bpls.cmake.gen
  @ONLY
)
file(GENERATE
  OUTPUT ${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake
  INPUT ${PROJECT_BINARY_DIR}/bpls.cmake.gen
)

# ADIOS_REORGANIZE
add_executable(adios_reorganize 
               ./adios_reorganize/main.cpp 
               ./adios_reorganize/Reorganize.cpp 
               Utils.cpp)
target_link_libraries(adios_reorganize adios2)
install(TARGETS adios_reorganize EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Add MPI dependency to parallel tools
if(ADIOS2_HAVE_MPI)
  target_link_libraries(adios_reorganize MPI::MPI_C)
endif()

if(ADIOS2_HAVE_MPI)
  add_subdirectory(adios_iotest)
endif()

