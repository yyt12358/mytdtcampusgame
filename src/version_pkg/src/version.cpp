#include "rclcpp/rclcpp.hpp"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <tdt_interface/msg/auto_aim_frame.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <tdt_interface/msg/auto_aim_frame.hpp>
#include <tdt_interface/msg/send_data.hpp>

using std::placeholders::_1;




const int minAreaValue = 70;          // 灯条最小面积
const int maxAreaValue = 4000;        // 灯条最大面积
const int maxAngleValue = 30;         // 灯条最大允许角度
const int maxCenterAngleValue = 20;   // 两个灯条中心连线最大允许角度
const double maxDisRatioValue = 4.0;  // 两个灯条之间的距离和灯条高度的比例


const float object_W = 0.135;//实际装甲版宽度
const float object_H = 0.055;//实际灯条长度

const int blueHMin = 90;
const int blueSMin = 150;
const int blueVMin = 150;
const int blueHMax = 132;

const int blueMinAreaValue = 3500;
const int blueMinWidthValue = 16;
const float blueMaxRatioValue = 2.0f;
const float blueMinCircValue = 0.60f;

int redHMin = 5, redHMax = 30;
int redSMin = 200, redSMax = 255;
int redVMin = 200, redVMax = 255;

int redHMin2 = 170, redHMax2 = 180;



struct lightbar
{
    cv::Point2f center;   // 储存灯条的中心点
    float width;      // 储存灯条的宽度
    float height;     // 储存灯条的高度
    float angle;      // 储存灯条的倾斜角度
    cv::Point2f top;      // 储存灯条上边的中心点
    cv::Point2f bottom;   // 储存灯条下边的中心点
    cv::Point2f corners[4]; // 储存灯条的四个角点
}; // 用来储存灯条的信息

struct Armor
{
    cv::Point2f center;   // 储存装甲板的中心点
    std::vector<cv::Point2f> corners;   // 储存装甲板的四个角点
    std::vector<cv::Point2f> pnpCorners; // 用来储存后面计算的pnp 的二维坐标点
    lightbar left;    // 储存装甲板左边的灯条
    lightbar right;   // 储存装甲板右边的灯条
}; // 用来储存装甲板的信息




double pointdistance(cv::Point2f p1, cv::Point2f p2)
{
    return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

//计算点的距离



class VersionNode : public rclcpp::Node //创造节点类
{
    public:
        VersionNode () : Node("version_node")       //构造函数
        {
            sub = this->create_subscription<sensor_msgs::msg::CompressedImage>("/camera/image_raw/compressed",10,std::bind(&VersionNode::frame_callback,this,_1)); //创建订阅者

            info_sub = this ->create_subscription<tdt_interface::msg::AutoAimFrame>("/autoaim/frame",10,std::bind(&VersionNode::info_callback,this,_1));

            send_angles = this ->create_publisher<tdt_interface::msg::SendData>("/target_angles_player_1",10);

            RCLCPP_INFO(this->get_logger(),"waiting...");
        }
        
        //创建节点订阅图像话题










    private:






    rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr sub;//声明要订阅

    rclcpp::Subscription<tdt_interface::msg::AutoAimFrame>::SharedPtr info_sub;


    bool judge =false;
    rclcpp::Publisher<tdt_interface::msg::SendData>::SharedPtr send_angles;

    std::vector<cv::Point3f> object_points = {
        {-object_W/2,-object_H/2,0},
        {object_W/2,-object_H/2,0},
        {object_W/2,object_H/2,0},
        {-object_W/2,object_H/2,0}
    };

    std::vector<double> distcoeffs;

    double fx =0;
    double fy = 0;
    double cx = 0;
    double cy = 0;


    double self_yaw;
    double self_pitch;
    int ammo;
    bool weakened;


    double vx;
    double vy;

    double sweep = 0.0;
    double up = 0.0;

    int nonecount = 0;



    // 扫视相关
    double sweep_yaw = 0.0;      // 当前扫视的绝对角度
    int sweep_dir = 1;           // 扫视方向: 1向右, -1向左
    int sweep_wait = 0;          // 到边界时的停顿帧数

    // Pitch 扫视相关 (新增)
    double sweep_pitch = 0.0;    // 当前俯仰角
    int pitch_dir = 1;           // Pitch 方向: 1向下, -1向上

    // 状态保持
    double last_target_yaw = 0.0; // 记住目标最后消失的角度
    bool has_target = false; // 之前是否看到过目标
    int lost_trg = 0;          // 目标丢失帧

    // 记录上一帧的角度和时间
    double last_rune_y = 0.0;
    double last_rune_p = 0.0;
    rclcpp::Time last_rune_time;
    bool has_last_rune = false;

    double fill_ratio;//？
    int blueshootcalm = 0;
    cv::Mat blueMask;

    int bluehold = 0;

    
    

    //声明成员变量

   bool runeAim(cv::Mat &Frame, tdt_interface::msg::SendData &out)
{
    cv::Mat hsv1;
    cv::cvtColor(Frame, hsv1, cv::COLOR_BGR2HSV);

    //提取亮蓝色
    cv::inRange(hsv1, cv::Scalar(blueHMin, blueSMin, blueVMin),
                     cv::Scalar(blueHMax, 255, 255), blueMask);

    // 形态学
    cv::Mat kernelBlue = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)); 
    cv::morphologyEx(blueMask, blueMask, cv::MORPH_CLOSE, kernelBlue); // 连接断裂线

    // 3. 找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(blueMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Point2f> runeCenters;
    std::vector<double> runeAreas;

    for (size_t i = 0; i < contours.size(); i++)
    {
        double area = cv::contourArea(contours[i]);
        
        // 面积太小R和噪点
        if (area < blueMinAreaValue) {continue;} 

        //计算圆度（越接近1越圆）
        double peri = cv::arcLength(contours[i], true);
        if (peri < 1.0) continue;
        double circ = 4 * CV_PI * area / (peri * peri);
        
        // 圆度必须大于 0.65 (靶心和圆环圆度都很高)
        if (circ < 0.65) {continue;} 

        cv::RotatedRect rect = cv::minAreaRect(contours[i]);


        double rect_area = rect.size.width * rect.size.height;
        if (rect_area < 1.0) {continue;} // 防止除以 0
        fill_ratio = area / rect_area;

        runeCenters.push_back(rect.center);

        runeAreas.push_back(area);

       

        if (fill_ratio < 0.75) {
        continue; 
        }//？

        // 可视化调参用：画出圆环的外接矩形
        cv::Point2f rp[4];
        rect.points(rp);
        for (int k = 0; k < 4; k++) {
            cv::line(Frame, rp[k], rp[(k+1)%4], cv::Scalar(0, 255, 255), 1);
        }
    }

    if (runeCenters.empty()) 
    {
        has_last_rune = false; 
        return false;
    }

    //找出离画面中心最近的圆环
    int best = 0;
    double bestDis = 1e10;
    for (size_t i = 0; i < runeCenters.size(); i++)
    {
        double d = pointdistance(runeCenters[i], cv::Point2f((float)cx, (float)cy));
        if (d < bestDis) { 
            bestDis = d; 
            best = (int)i; 
        }
    }

   

    cv::Point2f p = runeCenters[best];
    cv::drawMarker(Frame, p, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 24, 2);

    //计算相机的相对角度
    double yaw_cam = std::atan2(p.x - cx, fx) * 180 / CV_PI;
    double pitch_cam = std::atan2(p.y - cy, fy) * 180 / CV_PI;

    double raw_yaw_target = self_yaw + yaw_cam;
    double raw_pitch_target = self_pitch + pitch_cam;

    // 预测提前量
    auto now = this->get_clock()->now();
    double predicted_y = raw_yaw_target;
    double predicted_p = raw_pitch_target;

    if (has_last_rune) {
        double dt = (now - last_rune_time).seconds();
        if (dt > 0.01 && dt < 0.5) {
            double yaw_rate = (raw_yaw_target - last_rune_y) / dt;
            double pitch_rate = (raw_pitch_target - last_rune_p) / dt;

            // 预测提前
            predicted_y += yaw_rate * 0.03;
            predicted_p += pitch_rate * 0.03;
        }
    }

    last_rune_y = raw_yaw_target;
    last_rune_p = raw_pitch_target;
    last_rune_time = now;
    has_last_rune = true;

    RCLCPP_INFO(this->get_logger(),"fill_ratio:%.2f",fill_ratio);

    //.下发指令
    double y_err = std::abs(predicted_y- self_yaw);
    double p_err = std::abs(predicted_p- self_pitch);
    
    out.yaw = predicted_y;
    out.pitch = predicted_p - 1; 
    bool can_shoot = (y_err < 4.0 && p_err < 4.0 && ammo > 0 && !weakened);
    if(can_shoot && blueshootcalm == 0)
    {

    out.if_shoot = true;
    blueshootcalm = 0;

    }

else
{
out.if_shoot = false;
}   
 return true;
}





   
    void frame_callback (const sensor_msgs::msg::CompressedImage::SharedPtr msg )

    {

       
        if(blueshootcalm > 0)
        {
            blueshootcalm--;
        }

        if (bluehold > 0)
        {
            bluehold--;
        }

       
        if(!judge)
        {
            RCLCPP_WARN(this->get_logger(),"!");
                return;
        }

        

        auto stamp = msg->header.stamp;//传入时间戳

        auto now = this->get_clock()->now();//当前时间

        double age = (now - stamp).seconds();//计算帧龄

        if (age > 0.5)
        {
            RCLCPP_WARN(this->get_logger(),"too old.age:%.2f",age);

            return;
        }
        //帧龄检测（大于0.5排掉）

        const auto &jimg = *msg;

        std::vector<uchar> scene (jimg.data.begin(),jimg.data.end());//记录图像的字节流
        
        cv::Mat Frame = cv::imdecode(scene,cv::IMREAD_COLOR);//转opencv

        if (Frame.empty())//
        {
            RCLCPP_ERROR(this->get_logger(),"jiema failed");//

            return;//测一下图片有没有被调出
        }

        //不解码了直接调

        cv::Mat hsv; //声明hsv
        cv::Mat mask1,mask2,mask;//红色掩膜两个m合成一个大m

        cv::cvtColor(Frame,hsv,cv::COLOR_BGR2HSV);

        cv::inRange(hsv, cv::Scalar(redHMin, redSMin, redVMin), cv::Scalar(redHMax, redSMax, redVMax), mask1);

        cv::inRange(hsv, cv::Scalar(redHMin2, redSMin, redVMin), cv::Scalar(redHMax2, redSMax, redVMax), mask2);
        
        mask = mask1|mask2;//合成

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 9));
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);//形态学检测，去掉外部白色噪点
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,3)));

        //掩膜找红色

        std::vector<std::vector<cv::Point>> contours;//轮廓点组的组
        cv::findContours(mask,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);//找轮廓
        //RCLCPP_INFO(this->get_logger(), "contours: %zu", contours.size());//检测一下有没有识别到


        std::vector<lightbar> lightbars;//灯条数组
        std::vector<Armor> armors;  //装甲组     


        //鉴定灯条

        for (size_t i = 0;i < contours.size();i++)
        {
            double area = cv::contourArea(contours[i]);//计算长方形面积

            cv::RotatedRect rect = cv::minAreaRect(contours[i]);//找到最小外接长方形

            double height = rect.size.height; 
            
            double width = rect.size.width;  //记录高和宽

            if (width > height)
            {
                std::swap(width, height); 
            }

            //现在长是长，宽是宽

            cv::Point2f pts[4];//点组  存放一下矩形的点
            rect.points(pts);//存入

            double alength[4];//边长记录

            int numshort = 0;//初始化短边

            for(int i = 0;i < 4;i++)//遍历点组算边长
            {
                alength[i] = pointdistance(pts[i],pts[(i+1)%4]);
            }

            for(int i = 0;i<4;i++)
            {
                if(alength[i] < alength[(i+1)%4])
                {
                    numshort = i;//找一下短边号
                }
            }

            int oppositeshort = (numshort + 2) % 4; //短边的对边是短边

            cv::Point2f point1 = (pts[numshort] + pts[(numshort+1)%4]) / 2;//短边中点

            cv::Point2f point2 = (pts[oppositeshort] + pts[(oppositeshort+1)%4]) / 2;//对边短边中点

            

            cv::Point2f high;
            cv::Point2f low;

            if (point1.y < point2.y) 
            {
                high = point1;
                
                low = point2;
            }
            else
            {
                high = point2;

                low = point1;
            }

            // 根据两个短边中心点的y坐标判断上下关系，（高中点和低中点）

            double dx = high.x - low.x;
            double dy = low.y - high.y;
            //中点差距（好算位置）
            
            lightbar bar;
            bar.top = high;//灯条上边中点
            bar.bottom = low;//灯条下边中点
            bar.center = rect.center;//灯条中心
            bar.height = height;//灯条高度
            bar.width = width;//灯条宽度

            for(int i = 0; i <4; i++)
            {
               bar.corners[i] = pts[i];
            }
            bar.angle = atan2(dx, dy) * 180 / CV_PI;//灯条与竖直方向夹角

            //

            //RCLCPP_INFO(this->get_logger(),"area=%.1f, h/w=%.2f, angle=%.2f",area, height/width, bar.angle);

            if (height / width > 1.2&& area > minAreaValue && area < maxAreaValue && abs(bar.angle) < maxAngleValue)
            {
                lightbars.push_back(bar);
                // 直接画 RotatedRect，不管 pts 顺序
                cv::Point2f rect_pts[4];
                rect.points(rect_pts);
                for (int k = 0; k < 4; k++) {
                cv::line(Frame, rect_pts[k], rect_pts[(k+1)%4], cv::Scalar(255, 0, 0), 2);
                }

             // 画上下中心点
                cv::circle(Frame, high, 5, cv::Scalar(0, 255, 0), -1);
                cv::circle(Frame, low, 5, cv::Scalar(255, 0, 0), -1);
            }

            //识别并画出了灯条



            
        }

        //开始做灯条的配对和装甲识别
            for (int i = 0; i<lightbars.size();i++)
            {
                for(int j = i+1; j <lightbars.size();j++)
                {
                    auto &a = lightbars[i];
                    auto &b = lightbars[j];

                    double center_dis = pointdistance(a.center,b.center);
                    if (center_dis < 50|| center_dis > 2000 )//灯条中心距离
                    {
                        continue;
                    }

                    double high_diff_rate = abs(a.center.y - b.center.y)/std::max(a.center.y,b.center.y);
                    if(high_diff_rate > 0.35)//高度差比例
                    {
                        continue;
                    }

                    double angle_diff = abs(a.angle - b.angle);//倾斜角度差
                    if(angle_diff > 20)
                    {
                        continue;
                    }

                    double centerDx = a.center.x - b.center.x; 
                    double centerDy = a.center.y - b.center.y; 
                    double centerAngle = atan2(centerDy, centerDx) * 180 / CV_PI; //中心角度差

                    if(centerAngle > 25)
                    {
                        continue;
                    }
                    

                    Armor armor;

                    if(a.center.x < b.center.x)
                    {
                        armor.left = a;
                        armor.right = b;
                    }
                    else{
                        armor.left = b;
                        armor.right = a;
                    }


                    armor.center = (armor.left.center + armor.right.center)/2;
                    armor.corners.push_back(armor.left.top);
                    armor.corners.push_back(armor.right.top);
                    armor.corners.push_back(armor.right.bottom);
                    armor.corners.push_back(armor.left.bottom);//记入点顺序，左上 右上 右下 左上

                    cv::circle(Frame,armor.center,5,cv::Scalar(0,0,255),2,-1,cv::LINE_AA);


                    //装甲配对完毕
                    armor.pnpCorners = armor.corners;
                    armors.push_back(armor);
                }
            }//外层for


             if (!armors.empty()) {
                nonecount = 0;
                RCLCPP_INFO(this->get_logger(), "num=%.d", armors.size());
             auto &armor = armors[0];

             std::vector<cv::Point2f> imagePoints = armor.pnpCorners;//装甲板角点

             cv::Mat K = (cv::Mat_<double>(3,3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
             cv::Mat dist = cv::Mat(distcoeffs).clone();
             cv::Mat rvec, tvec;
             bool ok = cv::solvePnP(object_points,imagePoints, K, dist, rvec, tvec);
             if (ok) {
            double xc = tvec.at<double>(0);
            double yc = tvec.at<double>(1);
            double zc = tvec.at<double>(2);

            double yaw_cam = atan2(xc, zc) * 180 / CV_PI;
            double pitch_cam = atan2(yc, zc) * 180 / CV_PI;

            double yaw_target = self_yaw + yaw_cam;
            double pitch_target = self_pitch + pitch_cam;

            double y_err = std::abs(yaw_target - self_yaw);
            double p_err = std::abs(pitch_target - self_pitch);
            bool can = y_err < 2.0 && p_err < 2 && ammo > 0 && !weakened;

            auto ang = tdt_interface::msg::SendData();

            
           
            // 记录目标最后出现的位置
            last_target_yaw = yaw_target;
            has_target= true;
            lost_trg= 0;  // 重置丢失计数


            // 保存上一帧的角度
            static double last_yaw = 0.0;
            

            // 算变化量
            double yaw_delta = yaw_target - last_yaw;
            RCLCPP_INFO(this->get_logger(), "yaw:%.2f",yaw_delta);
            last_yaw = yaw_target;



            if(std::abs(yaw_delta) > 0.07)
            {
             // 提前量：把当前角度再往前推一点
              RCLCPP_INFO(this->get_logger(),"yumiao");
            yaw_target += yaw_delta *5;
            ang.yaw = yaw_target;
            ang.pitch = pitch_target - 1.0;
            ang.if_shoot = can;
            send_angles->publish(ang);
            }


            

            else{

            yaw_target += yaw_delta *0.3;

            ang.yaw = yaw_target;
            ang.pitch = pitch_target - 1.0;
            ang.if_shoot = can;
            send_angles->publish(ang);}

            
            sweep = self_yaw;
            

            }
        } //射击







    else { 
    // 1. 先尝试找能量机关的蓝色圆环
    tdt_interface::msg::SendData runeAng;
    if (runeAim(Frame, runeAng)) {
        nonecount = 0;

        bluehold = 50;
        send_angles->publish(runeAng);
        //cv::imshow("framebl", Frame);
        //cv::imshow("blue", blueMask);
        //cv::waitKey(1);
        return;
    }

    // 2. 没找到任何目标，开始搜索逻辑
    nonecount++;
    lost_trg++;

    auto ang = tdt_interface::msg::SendData();


    // 如果刚才看到过目标，且丢失时间少于 100 帧
    if (has_target&& lost_trg < 100) {
        RCLCPP_INFO(this->get_logger(), "最后位置:%.1f", last_target_yaw);
        
        ang.yaw = last_target_yaw;  // 最后消失地方
        ang.pitch = self_pitch;     // 保持当前俯仰
        ang.if_shoot = false;
        send_angles->publish(ang);
        
      
        return;  //直接返回
    } else {
        // 盯了 100 帧还没出来
        has_target = false; 
    }

    // 摆头阶段 
    if (std::abs(vx) <= 0.1 && std::abs(vy) <= 0.1) {
        RCLCPP_INFO(this->get_logger(), "大范围扫视中:%.1f", sweep_yaw);

        // 如果正在停顿
        if (sweep_wait > 0) {
            sweep_wait--;
        } else {
            // 正常扫视 (每帧 0.4 度)
            sweep_yaw += sweep_dir * 0.4; 
            
            // 扫到右边边界
            if (sweep_yaw >= 55.0) {
                sweep_yaw = 55.0;
                sweep_dir = -1.5;
                sweep_wait = 20; // 停顿
            }
            // 扫到左边边界
            if (sweep_yaw <= -25.0) {
                sweep_yaw = -25.0;
                sweep_dir = 1.5;
                sweep_wait = 20;
            }
        }
         // Pitch 上下扫 
    // Pitch 扫描速度慢一点，每帧 0.1 度
        sweep_pitch += pitch_dir * 0.15;
    
        if (sweep_pitch >= 5.0) {
            sweep_pitch = 5.0;
            pitch_dir = -1;
        }
        if (sweep_pitch <= -35.0) {
            sweep_pitch = -35.0;
            pitch_dir = 1;
        }
        double finalpitch = sweep_pitch;

        if(bluehold > 0)
        {
            finalpitch = -45;
        }

    RCLCPP_INFO(this->get_logger(), "扫视: yaw=%.1f, pitch=%.1f", sweep_yaw, sweep_pitch);

        // 防止看地
        ang.yaw = sweep_yaw;  
        ang.pitch =finalpitch;      
        ang.if_shoot = false;
        send_angles->publish(ang);
    } 
    else {
        // 底盘动了 → 云台保持当前角度
        RCLCPP_INFO(this->get_logger(), "底盘移动，云台定住");
        ang.yaw = self_yaw;
        ang.pitch = self_pitch;
        ang.if_shoot = false;
        send_angles->publish(ang);
    }
}
                           




        

        cv::imshow("frame",Frame);

        cv::imshow("mask",mask);

        cv::waitKey(1);

    }




    void info_callback(const tdt_interface::msg::AutoAimFrame::SharedPtr msg)
    {
        const auto &n=msg->camera_info.k;
        fx = n[0];
        fy = n[4];
        cx = n[2];
        cy = n[5];//存内参
        distcoeffs = msg->camera_info.d;//存畸变
        

        const auto &m = msg->state;
        double self_x = m.twist.linear.x;
        double self_y = m.twist.linear.y;

        self_yaw = m.yaw_degrees;
        self_pitch = m.pitch_degrees;

        ammo = m.ammo_remaining;
        weakened = m.is_weakened;

        vx = m.twist.linear.x;
        vy = m.twist.linear.y;



        judge = true;//走这的状态判断

    }
};



int main(int argc, char**argv)
{

    rclcpp::init(argc, argv); // 初始化ROS 2

    cv::namedWindow("frame",cv::WINDOW_NORMAL);
    cv::namedWindow("mask",cv::WINDOW_NORMAL);
    cv::namedWindow("bar",cv::WINDOW_NORMAL);

    cv::createTrackbar("R_HMin", "bar", &redHMin, 180);
    cv::createTrackbar("R_HMax", "bar", &redHMax, 180);
    cv::createTrackbar("R_SMin", "bar", &redSMin, 255);
    cv::createTrackbar("R_VMin", "bar", &redVMin, 255);
    
    cv::createTrackbar("R_HMin2", "bar", &redHMin2, 180);
    cv::createTrackbar("R_HMax2", "bar", &redHMax2, 180);


    auto node = std::make_shared<VersionNode>(); // 创建校园赛节点

    rclcpp::spin(node); // 让节点持续运行并等待ROS 2消息

    rclcpp::shutdown(); // 关闭ROS 2

    return 0;
}
