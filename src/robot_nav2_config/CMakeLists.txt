cmake_minimum_required(VERSION 3.8)
project(robot_nav2_config)

find_package(ament_cmake REQUIRED)

ament_package()

install(DIRECTORY launch map param rviz
  DESTINATION share/${PROJECT_NAME})
