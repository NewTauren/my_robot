import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
      config = os.path.join(
          get_package_share_directory('robot_navigation'),
          'config',
          'waypoints.yaml')

      return LaunchDescription([
          DeclareLaunchArgument('use_sim_time',default_value='false'),
          Node(
              package='robot_navigation',
              executable='waypoint_navigator',
              name='waypoint_navigator',
              parameters=[config, {'use_sim_time': LaunchConfiguration('use_sim_time')}],
              output='screen'),
      ])
