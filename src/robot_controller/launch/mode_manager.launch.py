from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
      return LaunchDescription([
          Node(
              package='robot_controller',
              executable='mode_manager',
              name='mode_manager',
              output='screen',
              parameters=[{
                  'default_mode': 'NAV',
              }]
          )
      ])
