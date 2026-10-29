#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

// 定义结构体，存储车的各项信息
struct Car {
    string name;    // 名字
    long long time; // 最快圈速
    int count;      // 圈数
};

// 自定义比较函数，用于第一轮按名字排序（把相同的车排到一起）
bool cmpName(const Car& a, const Car& b) {
    return a.name < b.name;
}

// 自定义比较函数，用于第二轮最终排序
bool cmpFinal(const Car& a, const Car& b) {
    if (a.time != b.time) {
        return a.time < b.time; 
    }
    return a.name < b.name; 
}

int main() {
    
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int m;
    cin >> m;
    
    vector<Car> cars(m);
    for (int i = 0; i < m; i++) {
        cin >> cars[i].name >> cars[i].time;
        cars[i].count = 1;
    }

    
    sort(cars.begin(), cars.end(), cmpName);

    //合并相同的车
    vector<Car> unique_cars;
    for (int i = 0; i < m; i++) {
        // 如果是第一辆车，或者名字和前面不同，说明是一辆新车
        if (i == 0 || cars[i].name != cars[i - 1].name) {
            unique_cars.push_back(cars[i]);
        } else {
            // 如果是同一辆车，合并到 unique_cars 的最后一辆车上
            int last_idx = unique_cars.size() - 1;
            
            // 更新最快圈速
            if (cars[i].time < unique_cars[last_idx].time) {
                unique_cars[last_idx].time = cars[i].time;
            }
            // 累加圈数
            unique_cars[last_idx].count += cars[i].count;
        }
    }


    sort(unique_cars.begin(), unique_cars.end(), cmpFinal);

    //  计算名次并输出
    int n = unique_cars.size();
    for (int i = 0; i < n; i++) {
        int rank;
        // 名次规则：严格比它成绩好的人数 + 1
        if (i == 0) {
            rank = 1; // 第一名
        } else {
            // 如果成绩和前面的人一样，名次就和前面那个人一样
            if (unique_cars[i].time == unique_cars[i - 1].time) {
                // 因为前面排好序了，我们只需要找前面第一个和它成绩不同的人
                int j = i - 1;
                while (j > 0 && unique_cars[j].time == unique_cars[i].time) {
                    j--;
                }
                rank = j + 2; // 名次等于前面不同成绩的人数 + 1 
            } else {
                rank = i + 1; // 成绩不同，名次就是当前排位 
            }
        }
        
        // 输出格式：名次 名字 最好成绩 圈数
        cout << rank << " " << unique_cars[i].name << " " 
             << unique_cars[i].time << " " << unique_cars[i].count << "\n";
    }

    return 0;
}