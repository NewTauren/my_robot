#!/usr/bin/env python3
import os
from ament_index_python.packages import get_package_share_directory, get_package_prefix
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, ExecuteProcess, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    grace_period = os.environ.get("GRACE_PERIOD", "10")

    use_sim_time = LaunchConfiguration("use_sim_time", default="true")
    nav_use_sim_time = LaunchConfiguration("nav_use_sim_time", default="true")
    default_mode = LaunchConfiguration("default_mode", default="NAV")

    robot_gazebo_dir = get_package_share_directory("robot_gazebo")
    nav2_config_dir = get_package_share_directory("robot_nav2_config")
    robot_controller_dir = get_package_share_directory("robot_controller")

    params_file = os.path.join(robot_controller_dir, "config", "params.yaml")
    map_file = os.path.join(nav2_config_dir, "map", "map.yaml")
    rc_lib = os.path.join(get_package_prefix("robot_controller"), "lib", "robot_controller")

    kill_old_gz = ExecuteProcess(
        cmd=['bash', '-c', 'killall -9 gzserver gzclient 2>/dev/null'],
        output='screen',
    )

    gazebo_launch = TimerAction(
        period=2.0,
        actions=[
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(robot_gazebo_dir, "launch", "indoor_cruise.launch.py")
                ),
                launch_arguments={"use_sim_time": use_sim_time}.items(),
            )
        ],
    )

    nav2_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(nav2_config_dir, "launch", "navigation2.launch.py")
        ),
        launch_arguments={
            "use_sim_time": nav_use_sim_time,
            "map": map_file,
            "params_file": params_file,
        }.items(),
    )

    visual_follower_node = Node(
        package="robot_vision",
        executable="visual_follower",
        name="visual_follower",
        output="screen",
        parameters=[params_file],
    )

    mode_manager_node = Node(
        package="robot_controller",
        executable="mode_manager",
        name="mode_manager",
        output="screen",
        parameters=[params_file],
    )

    relay_node = Node(
        package="robot_controller",
        executable="topic_relay.py",
        name="cmd_vel_relay",
        output="screen",
    )

    watchdog = TimerAction(
        period=10.0,
        actions=[
            ExecuteProcess(
                cmd=[os.path.join(rc_lib, "cleanup_watchdog.py"), grace_period],
                output="screen",
            )
        ],
    )

    return LaunchDescription([
        DeclareLaunchArgument("use_sim_time", default_value="true"),
        DeclareLaunchArgument("nav_use_sim_time", default_value="true"),
        DeclareLaunchArgument("default_mode", default_value="NAV"),

        kill_old_gz,
        gazebo_launch,
        visual_follower_node,
        mode_manager_node,
        relay_node,
        nav2_launch,
        watchdog,
    ])
