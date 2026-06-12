cmake_minimum_required(VERSION 3.8)
project(robot_gazebo)

find_package(ament_cmake REQUIRED)

ament_package()

install(DIRECTORY launch models worlds rviz
  DESTINATION share/${PROJECT_NAME})
