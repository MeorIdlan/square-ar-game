cmake_minimum_required(VERSION 3.25)

find_package(GTest CONFIG REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core)

# Helper: create a test executable linked to core + gtest
function(add_unit_test TEST_NAME SOURCE_FILE)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE
        square_ar_core
        GTest::gtest_main
    )
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_unit_test(test_grid_model          test_grid_model.cpp)
add_unit_test(test_round_evaluator     test_round_evaluator.cpp)
add_unit_test(test_floor_mapping       test_floor_mapping.cpp)
add_unit_test(test_detection_deduplicator test_detection_deduplicator.cpp)
add_unit_test(test_player_tracker      test_player_tracker.cpp)
add_unit_test(test_game_engine         test_game_engine.cpp)
add_unit_test(test_camera_format       test_camera_format.cpp)
add_unit_test(test_fps_tracker         test_fps_tracker.cpp)
