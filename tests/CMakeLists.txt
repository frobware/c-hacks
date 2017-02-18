add_executable(test-hashtbl test-hashtbl.c ../src/hashtbl.c)
add_test(test-hashtbl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-hashtbl)
target_compile_definitions(test-hashtbl PRIVATE "HASHTBL_MAX_TABLE_SIZE=((1<<8))")

add_executable(test-linked-hashtbl test-linked-hashtbl.c ../src/linked-hashtbl.c)
add_test(test-linked-hashtbl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-linked-hashtbl)
target_compile_definitions(test-linked-hashtbl PRIVATE "LINKED_HASHTBL_MAX_TABLE_SIZE=((1<<8))")
