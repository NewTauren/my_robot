#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <vector>
#include <cmath>

  class WaypointNavigator : public rclcpp::Node
  {
  public:
      using NavigateToPose = nav2_msgs::action::NavigateToPose;
      using GoalHandle = rclcpp_action::ClientGoalHandle<NavigateToPose>;

      WaypointNavigator() : Node("waypoint_navigator"), current_idx_(0)
      {
          declare_parameter<std::vector<double>>("waypoints", std::vector<double>());
          waypoints_ = get_parameter("waypoints").as_double_array();

          if (waypoints_.empty() || waypoints_.size() % 4 != 0) {
              RCLCPP_ERROR(get_logger(), "waypoints must be [x,y,z,yaw, x,y,z,yaw, ...]");
              return;
          }

          client_ = rclcpp_action::create_client<NavigateToPose>(this, "/navigate_to_pose");

          RCLCPP_INFO(get_logger(), "Waiting for Nav2 action server...");
          client_->wait_for_action_server();
          RCLCPP_INFO(get_logger(), "Nav2 connected! %zu waypoints loaded.", waypoints_.size() / 4);

          timer_ = create_wall_timer(std::chrono::seconds(1), [this]() {
              timer_->cancel();
              send_next();
          });
      }

  private:
      void send_next()
      {
          if (current_idx_ >= waypoints_.size() / 4) {
              RCLCPP_INFO(get_logger(), "All waypoints reached! Shutting down.");
              rclcpp::shutdown();
              return;
          }

          size_t i = current_idx_ * 4;
          double x = waypoints_[i];
          double y = waypoints_[i + 1];
          double z = waypoints_[i + 2];
          double yaw = waypoints_[i + 3];

          RCLCPP_INFO(get_logger(), "-> Waypoint %zu: (%.2f, %.2f, %.2f, yaw=%.2f)",
                      current_idx_ + 1, x, y, z, yaw);

          auto goal = NavigateToPose::Goal();
          goal.pose.header.frame_id = "map";
          goal.pose.header.stamp = now();
          goal.pose.pose.position.x = x;
          goal.pose.pose.position.y = y;
          goal.pose.pose.position.z = z;
          goal.pose.pose.orientation.z = std::sin(yaw * 0.5);
          goal.pose.pose.orientation.w = std::cos(yaw * 0.5);

          auto opts = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
          opts.result_callback = [this](const GoalHandle::WrappedResult &r) {
              if (r.code == rclcpp_action::ResultCode::SUCCEEDED) {
                  RCLCPP_INFO(get_logger(), "Waypoint %zu reached!", current_idx_ + 1);
              } else {
                  RCLCPP_WARN(get_logger(), "Waypoint %zu failed, skipping.", current_idx_ + 1);
              }
              current_idx_++;
              send_next();
          };
          client_->async_send_goal(goal, opts);
      }

      rclcpp_action::Client<NavigateToPose>::SharedPtr client_;
      std::vector<double> waypoints_;
      size_t current_idx_;
      rclcpp::TimerBase::SharedPtr timer_;
  };

  int main(int argc, char **argv)
  {
    rclcpp::init(argc,argv);
    rclcpp::spin(std::make_shared<WaypointNavigator>());
    rclcpp::shutdown();
    return 0;
  }