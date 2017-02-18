set(SRCS
  btree.c
  hashtbl.c
  linked-hashtbl.c)

add_library(${CHACKS_LIB_NAME} STATIC ${SRCS})
