SET(this_target PiCam)


INCLUDE_DIRECTORIES(
  ${Boost_INCLUDE_DIRS}
  ${Internal_INCLUDE}
)

SET(${this_target}_HEADER_FILES
  Operations.h
  ActiveObject.h
  CameraAndLightControl.h
  TcpServer/ISender.h
  TcpServer/TcpConnection.h
  TcpServer/TcpServer.h
  Subject.h
  IObserver.h
  RemoteControl.h
  Notifier.h
  sqlite/sqlite3.h
  Statement.h
  SendEmail.h
)

SET( ${this_target}_SOURCE_FILES
  main.cpp
  sqlite/sqlite3.c
) 

#SET_SOURCE_FILES_PROPERTIES(${this_target}_HEADER_FILES
#                            PROPERTIES HEADER_FILE_ONLY TRUE)
 
LIST(APPEND ${this_target}_SOURCE_FILES ${${this_target}_HEADER_FILES})

add_executable(${this_target} ${${this_target}_SOURCE_FILES})
target_link_libraries(${this_target} ${Boost_LIBRARIES})
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries(${this_target} pthread)
target_link_libraries(${this_target} dl)
ENDIF()
