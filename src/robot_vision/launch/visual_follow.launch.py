import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
      return LaunchDescription([
          Node(
              package='robot_vision',
              executable='visual_follower',
              name='visual_follower',
              output='screen',
              parameters=[{
                  'linear_scale': 0.4,
                  'angular_scale': 1.2,
              }]
          )
      ])
