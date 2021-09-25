project(first_person_shooter_lol)

file (GLOB_RECURSE C_FILES *.cpp)
file (GLOB_RECURSE H_FILES *.h)
set (SOURCE_FILES ${C_FILES} ${H_FILES})
add_executable(first_person_shooter_lol ${C_FILES})

target_link_libraries(first_person_shooter_lol raylib)
