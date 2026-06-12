#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

class VisualFollower : public rclcpp::Node
  {
  public:
      VisualFollower() : Node("visual_follower")
      {
          declare_parameter("linear_scale", 0.4);
          declare_parameter("angular_scale", 1.2);
          declare_parameter("cmd_vel_topic", "/cmd_vel_vision");

          linear_scale_ = get_parameter("linear_scale").as_double();
          angular_scale_ = get_parameter("angular_scale").as_double();

          cmd_vel_pub_ = create_publisher<geometry_msgs::msg::Twist>(
              get_parameter("cmd_vel_topic").as_string(), 10);

          image_sub_ = create_subscription<sensor_msgs::msg::Image>(
              "/camera/image_raw", 10,
              std::bind(&VisualFollower::image_callback, this, std::placeholders::_1));

          RCLCPP_INFO(get_logger(), "Visual Follower started");
      }

  private:
      void image_callback(const sensor_msgs::msg::Image::SharedPtr msg)
      {
          cv::Mat frame;
          try {
              frame = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8)->image;
          } catch (const std::exception &e) {
              RCLCPP_ERROR(get_logger(), "cv_bridge error: %s", e.what());
              return;
          }

          cv::Mat hsv;
          cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

          // Red has two HSV ranges (wraps around hue 0)
          cv::Mat mask1, mask2, mask;
          cv::inRange(hsv, cv::Scalar(0, 70, 50), cv::Scalar(10, 255, 255), mask1);
          cv::inRange(hsv, cv::Scalar(170, 70, 50), cv::Scalar(180, 255, 255), mask2);
          cv::addWeighted(mask1, 1.0, mask2, 1.0, 0.0, mask);

          cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
          cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);

          std::vector<std::vector<cv::Point>> contours;
          cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

          geometry_msgs::msg::Twist cmd;

          if (contours.empty()) {
              cmd.linear.x = 0.0;
              cmd.angular.z = 0.0;
              cmd_vel_pub_->publish(cmd);
              return;
          }

          // Pick largest red blob
          int largest = 0;
          double max_area = 0;
          for (size_t i = 0; i < contours.size(); i++) {
              double area = cv::contourArea(contours[i]);
              if (area > max_area) {
                  max_area = area;
                  largest = i;
              }
          }

          cv::Rect rect = cv::boundingRect(contours[largest]);
          int cx = rect.x + rect.width / 2;

          int img_cx = msg->width / 2;
          int cx_error = cx - img_cx;
          double area_ratio = max_area / (msg->width * msg->height);

          cmd.angular.z = -angular_scale_ * (static_cast<double>(cx_error) / img_cx);

          if (area_ratio > 0.25) {
              cmd.linear.x = 0.0;
          } else if (area_ratio > 0.10) {
              cmd.linear.x = linear_scale_ * 0.3;
          } else {
              cmd.linear.x = linear_scale_;
          }
          cmd.linear.x *= (1.0 - std::min(std::abs(cmd.angular.z), 1.0) * 0.5);

          RCLCPP_DEBUG(get_logger(), "err=%d area=%.3f lin=%.2f ang=%.2f",
                       cx_error, area_ratio, cmd.linear.x, cmd.angular.z);

          cmd_vel_pub_->publish(cmd);
      }

      rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
      rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
      double linear_scale_;
      double angular_scale_;
  };


int main(int argc,char**argv)
{
    rclcpp::init(argc,argv);
    auto node =std::make_shared<VisualFollower>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}