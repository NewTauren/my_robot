#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/string.hpp>

class ModeManager : public rclcpp::Node
  {
  public:
      ModeManager() : Node("mode_manager"), mode_("NAV"), last_nav_time_(now()), last_vision_time_(now())
      {
          declare_parameter("default_mode", "NAV");
          mode_ = get_parameter("default_mode").as_string();

          cmd_vel_pub_ = create_publisher<geometry_msgs::msg::Twist>("/cmd_vel_muxed", 10);

          nav_sub_ = create_subscription<geometry_msgs::msg::Twist>(
              "/cmd_vel", 10,
              std::bind(&ModeManager::nav_callback, this, std::placeholders::_1));

          vision_sub_ = create_subscription<geometry_msgs::msg::Twist>(
              "/cmd_vel_vision", 10,
              std::bind(&ModeManager::vision_callback, this, std::placeholders::_1));

          mode_sub_ = create_subscription<std_msgs::msg::String>(
              "/mode_switch", 10,
              std::bind(&ModeManager::mode_callback, this, std::placeholders::_1));

          // Watchdog: if no cmd_vel from active source for 0.5s, publish zero
          watchdog_ = create_wall_timer(std::chrono::milliseconds(500), [this]() {
              auto now_time = now();
              if ((mode_ == "NAV" && (now_time - last_nav_time_).seconds() > 0.5) ||
                  (mode_ == "VISION" && (now_time - last_vision_time_).seconds() > 0.5)) {
                  cmd_vel_pub_->publish(geometry_msgs::msg::Twist());
              }
          });

          RCLCPP_INFO(get_logger(), "Mode Manager started, mode: %s", mode_.c_str());
      }

  private:
      void nav_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
      {
          last_nav_time_ = now();
          last_nav_ = *msg;
          if (mode_ == "NAV") cmd_vel_pub_->publish(*msg);
      }

      void vision_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
      {
          last_vision_time_ = now();
          last_vision_ = *msg;
          if (mode_ == "VISION") cmd_vel_pub_->publish(*msg);
      }

      void mode_callback(const std_msgs::msg::String::SharedPtr msg)
      {
          std::string m = msg->data;
          if (m == "NAV" || m == "VISION" || m == "MANUAL") {
              if (mode_ != m) {
                  mode_ = m;
                  RCLCPP_INFO(get_logger(), "Switched to mode: %s", mode_.c_str());
                  // Immediately publish last known cmd from new mode (or zero)
                  if (mode_ == "NAV") cmd_vel_pub_->publish(last_nav_);
                  else if (mode_ == "VISION") cmd_vel_pub_->publish(last_vision_);
                  else cmd_vel_pub_->publish(geometry_msgs::msg::Twist());
              }
          } else {
              RCLCPP_WARN(get_logger(), "Unknown mode: %s (NAV/VISION/MANUAL)", m.c_str());
          }
      }

      std::string mode_;
      rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
      rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr nav_sub_;
      rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr vision_sub_;
      rclcpp::Subscription<std_msgs::msg::String>::SharedPtr mode_sub_;
      rclcpp::TimerBase::SharedPtr watchdog_;
      geometry_msgs::msg::Twist last_nav_, last_vision_;
      rclcpp::Time last_nav_time_, last_vision_time_;
  };

  int main(int argc, char **argv)
  {
      rclcpp::init(argc, argv);
      rclcpp::spin(std::make_shared<ModeManager>());
      rclcpp::shutdown();
      return 0;
  }