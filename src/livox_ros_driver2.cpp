//
// The MIT License (MIT)
//
// Copyright (c) 2022 Livox. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <iostream>
#include <chrono>
#include <vector>
#include <csignal>
#include <thread>

#include "include/livox_ros_driver2.h"
#include "include/ros_headers.h"
#include "driver_node.h"
#include "lddc.h"
#include "lds_lidar.h"

using namespace livox_ros;

namespace livox_ros
{
DriverNode::DriverNode(const rclcpp::NodeOptions & node_options)
: Node("livox_driver_node", node_options)
{
  DRIVER_INFO(*this, "Livox Ros Driver2 Version: %s", LIVOX_ROS_DRIVER2_VERSION_STRING);

  /** Init default system parameter */
  int xfer_format = kPointCloud2Msg;
  int multi_topic = 0;
  int data_src = kSourceRawLidar;
  double publish_freq = 10.0; /* Hz */
  int output_type = kOutputToRos;
  std::string frame_id;

  this->declare_parameter("xfer_format", xfer_format);
  this->declare_parameter("multi_topic", 0);
  this->declare_parameter("data_src", data_src);
  this->declare_parameter("publish_freq", 10.0);
  this->declare_parameter("output_data_type", output_type);
  this->declare_parameter("frame_id", "frame_default");
  this->declare_parameter("user_config_path", "path_default");
  this->declare_parameter("cmdline_input_bd_code", "000000000000001");
  this->declare_parameter("lvx_file_path", "/home/livox/livox_test.lvx");

  this->get_parameter("xfer_format", xfer_format);
  this->get_parameter("multi_topic", multi_topic);
  this->get_parameter("data_src", data_src);
  this->get_parameter("publish_freq", publish_freq);
  this->get_parameter("output_data_type", output_type);
  this->get_parameter("frame_id", frame_id);

  if (publish_freq > 100.0) {
    publish_freq = 100.0;
  } else if (publish_freq < 0.5) {
    publish_freq = 0.5;
  } else {
    publish_freq = publish_freq;
  }

  future_ = exit_signal_.get_future();

  /** Lidar data distribute control and lidar data source set */
  lddc_ptr_ = std::make_unique<Lddc>(xfer_format, multi_topic, data_src, output_type, publish_freq, frame_id);
  lddc_ptr_->SetRosNode(this);

  if (data_src == kSourceRawLidar) {
    DRIVER_INFO(*this, "Data Source is raw lidar.");

    std::string user_config_path;
    this->get_parameter("user_config_path", user_config_path);
    DRIVER_INFO(*this, "Config file : %s", user_config_path.c_str());

    std::string cmdline_bd_code;
    this->get_parameter("cmdline_input_bd_code", cmdline_bd_code);

    LdsLidar *read_lidar = LdsLidar::GetInstance(publish_freq);
    lddc_ptr_->RegisterLds(static_cast<Lds *>(read_lidar));

    if ((read_lidar->InitLdsLidar(user_config_path))) {
      DRIVER_INFO(*this, "Init lds lidar success!");
    } else {
      DRIVER_ERROR(*this, "Init lds lidar fail!");
    }
  } else {
    DRIVER_ERROR(*this, "Invalid data src (%d), please check the launch file", data_src);
  }

  pointclouddata_poll_thread_ = std::make_shared<std::thread>(&DriverNode::PointCloudDataPollThread, this);
  imudata_poll_thread_ = std::make_shared<std::thread>(&DriverNode::ImuDataPollThread, this);
}

}  // namespace livox_ros

#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE(livox_ros::DriverNode)


void DriverNode::PointCloudDataPollThread()
{
  std::future_status status;
  std::this_thread::sleep_for(std::chrono::seconds(3));
  do {
    lddc_ptr_->DistributePointCloudData();
    status = future_.wait_for(std::chrono::microseconds(0));
  } while (status == std::future_status::timeout);
}

void DriverNode::ImuDataPollThread()
{
  std::future_status status;
  std::this_thread::sleep_for(std::chrono::seconds(3));
  do {
    lddc_ptr_->DistributeImuData();
    status = future_.wait_for(std::chrono::microseconds(0));
  } while (status == std::future_status::timeout);
}





















