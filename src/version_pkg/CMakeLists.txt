cmake_minimum_required(VERSION 3.8)
project(version_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(compressed_image_transport REQUIRED)
find_package(camera_info_manager REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tdt_interface REQUIRED)
find_package(OpenCV REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()
add_executable(version_node src/version.cpp)

ament_target_dependencies(version_node
  rclcpp
  sensor_msgs
  image_transport
  compressed_image_transport
  camera_info_manager
  cv_bridge
  std_msgs
  geometry_msgs
  tdt_interface
  OpenCV
)

# 判断系统里有没有这个绝对路径的文件夹
if(EXISTS "/usr/local/opencv-4.7.0/lib")
    # 如果存在，比赛电脑环境，强行写死
    set(OpenCV_LIBS 
        "/usr/local/opencv-4.7.0/lib/libopencv_core.so.407"
        "/usr/local/opencv-4.7.0/lib/libopencv_imgproc.so.407"
        "/usr/local/opencv-4.7.0/lib/libopencv_highgui.so.407"
        "/usr/local/opencv-4.7.0/lib/libopencv_imgcodecs.so.407"
    )
    message(STATUS "使用比赛电脑指定的 OpenCV 4.7.0 绝对路径")
else()
    # 如果不存在，这是你自己的电脑，正常找
    find_package(OpenCV REQUIRED)
    message(STATUS "使用本机系统默认的 OpenCV")
endif()

target_link_libraries(version_node 
  ${OpenCV_LIBS} 
  opencv_core 
  opencv_imgproc 
  opencv_highgui 
  opencv_imgcodecs
)
install(TARGETS version_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
