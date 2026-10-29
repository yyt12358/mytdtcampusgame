#include <iostream>
#include <string>

using namespace std;

int main() {
    // 优化输入输出速度
    ios::sync_with_stdio(false);
    cin.tie(0);

    int END, n;
    // 读入终点位置和指令数
    if (!(cin >> END >> n)) return 0;

    int pos = 0;           // 初始位置在 0
    bool is_fast = false;  // 初始状态是普通车 (false)

    for (int i = 0; i < n; ++i) {
        char op;
        cin >> op; // 读入指令类型

        if (op == 'T') {
            // 切换模式
            is_fast = !is_fast; // 取反：普通变快车，快车变普通
            if (is_fast) {
                cout << "fast\n";
            } else {
                cout << "normal\n";
            }
        } 
        else if (op == 'F' || op == 'B') {
            int k;
            cin >> k; // 读入移动的格数
            
            // 计算实际移动距离
            int actual_move = is_fast ? 2 * k : k;
            
            // 根据前进或后退计算新位置
            int new_pos = pos;
            if (op == 'F') {
                new_pos += actual_move;
            } else {
                new_pos -= actual_move;
            }

            bool blocked = false;
            // 判断是否越界（超出终点或退过起点）
            if (new_pos > END) {
                new_pos = END;
                blocked = true;
            } else if (new_pos < 0) {
                new_pos = 0;
                blocked = true;
            }

            // 更新当前位置
            pos = new_pos;

            // 输出结果
            cout << pos;
            if (blocked) {
                cout << " blocked";
            }
            cout << "\n"; // 用 \n 比 endl 快
        }
    }

    return 0;
}