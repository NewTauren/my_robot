#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist


class CmdRelay(Node):
    def __init__(self):
        super().__init__("cmd_vel_relay")
        self.sub = self.create_subscription(Twist, "/cmd_vel_muxed", self.cb, 10)
        self.pub = self.create_publisher(Twist, "/cmd_vel", 10)
        self.get_logger().info("Relay /cmd_vel_muxed -> /cmd_vel started")

    def cb(self, msg):
        self.pub.publish(msg)


def main():
    rclpy.init()
    node = CmdRelay()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
