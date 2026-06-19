# ROS2 自主导航与视觉跟随机器人仿真

[![ROS2](https://img.shields.io/badge/ROS2-Humble-blue)](https://docs.ros.org/en/humble/)
[![Ubuntu](https://img.shields.io/badge/Ubuntu-22.04-orange)](https://ubuntu.com/)
[![Gazebo](https://img.shields.io/badge/Gazebo-11-green)](https://gazebosim.org/)

基于 TurtleBot3 Burger 模型，在 Gazebo 仿真环境中实现**自主导航（Nav2）**与**视觉目标跟随（OpenCV）**双模式切换的完整机器人系统。

![Demo](docs/demo.gif)

## 功能特性

- **自主导航模式** — 基于 Nav2 导航栈（AMCL 定位 + DWB 规划），支持多点航点巡航
- **视觉跟随模式** — 基于 OpenCV HSV 颜色检测，实时跟踪红色目标
- **模式仲裁** — NAV / VISION / MANUAL 三种模式实时切换，500ms 看门狗安全保护
- **一站式启动** — 一键启动 Gazebo 仿真 + 导航栈 + 视觉节点 + 进程管理

## 快速开始

### 环境要求

- Ubuntu 22.04 LTS
- ROS2 Humble（桌面完整版）
- Gazebo 11

```bash
# 安装 ROS2 Humble（如未安装）
sudo apt install ros-humble-desktop

# 安装依赖
sudo apt install ros-humble-nav2-bringup ros-humble-cartographer-ros \
                 ros-humble-dwb-plugins ros-humble-gazebo-ros-pkgs
sudo apt install libopencv-dev ros-humble-cv-bridge
```

### 编译

```bash
cd ~/robot_sim_project
colcon build
source install/setup.bash
```

### 运行

```bash
# 一键启动（Gazebo + Nav2 + 视觉 + 控制 + 看门狗）
export NEWBOT_MODEL=newbot
ros2 launch robot_controller integration.launch.py
```

## 使用指南

### 模式切换

```bash
# 视觉跟随模式（跟踪红色目标）
ros2 topic pub /mode_switch std_msgs/String "data: VISION" --once

# 自主导航模式
ros2 topic pub /mode_switch std_msgs/String "data: NAV" --once

# 急停
ros2 topic pub /mode_switch std_msgs/String "data: MANUAL" --once
```

### 航点导航

```bash
# 先启动集成环境，待 Gazebo + Nav2 就绪后：
ros2 launch robot_navigation waypoint_nav.launch.py
```

## 系统架构

```
                   ┌──────────────────┐
                   │  waypoint_navigator │
                   │  (Nav2 Action Client)│
                   └────────┬─────────┘
                            │ /cmd_vel_nav
                            ▼
┌──────────────┐   ┌──────────────────┐   ┌──────────────┐
│ visual_follower│──▶│   mode_manager   │──▶│  topic_relay  │──▶ /cmd_vel
│ (OpenCV/HSV)  │   │ NAV/VISION/MANUAL│   │ (Python桥接)  │      │
└──────────────┘   │  + Watchdog 500ms │   └──────────────┘      │
                   └──────────────────┘                           ▼
                                                          Gazebo diff_drive
```

## 项目结构

```
src/
├── robot_controller/     # 核心控制：mode_manager + watchdog + topic_relay
├── robot_vision/         # 视觉跟随：OpenCV HSV 红色检测
├── robot_navigation/     # 航点导航：Nav2 action client
├── robot_description/    # 机器人模型：URDF + STL
├── robot_gazebo/         # 仿真场景：indoor_cruise.world
├── robot_slam/           # Cartographer SLAM 配置
└── robot_nav2_config/    # Nav2 参数配置
```

## 关键设计

| 设计决策 | 说明 |
|---------|------|
| **模式仲裁** | mode_manager 分离控制源，NAV/VISION/MANUAL 互斥切换 |
| **话题桥接** | topic_relay 解决 Gazebo diff_drive 硬编码 `/cmd_vel` 问题 |
| **看门狗** | 500ms 超时自动停车，防止节点崩溃后小车失控 |
| **集中参数** | 所有参数统一在 `params.yaml` 管理，launch 时分发 |

## 技术栈

| 领域 | 技术 |
|------|------|
| 机器人建模 | URDF / TurtleBot3 Burger |
| 仿真环境 | Gazebo 11 + indoor_cruise 室内场景 |
| 导航规划 | Nav2（AMCL + DWB + Navfn） |
| 建图定位 | Cartographer SLAM |
| 视觉处理 | OpenCV 4（HSV 颜色空间） |
| 中间件 | ROS2 Humble（rclcpp / rclpy） |

## 许可证

MIT
