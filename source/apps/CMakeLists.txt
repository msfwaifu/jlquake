#############################################################################
# CLIENT/SERVER
#############################################################################

add_executable(jlquake
	../apps/main.cpp
	../apps/unix.cpp
)
set_target_properties(jlquake PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake ${CLIENTS_LIBS})

#############################################################################
# DEDICATED SERVER                        
#############################################################################

add_executable(jlquakeded
	../apps/main.cpp
	../apps/unix.cpp
)
set_target_properties(jlquakeded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlquakeded ${SERVERS_LIBS})
