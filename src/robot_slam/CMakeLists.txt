cmake_minimum_required(VERSION 3.8)
project(robot_slam)

find_package(ament_cmake REQUIRED)

ament_package()

install(DIRECTORY config launch rviz
  DESTINATION share/${PROJECT_NAME})
