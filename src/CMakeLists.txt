set(SRCS
  btree.c
  leb128.c
  hashtbl.c
  linked-hashtbl.c)

add_library(${CHACKS_LIB_NAME} STATIC ${SRCS})
