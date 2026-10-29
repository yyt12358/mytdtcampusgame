#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {
    
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int R, C;
    if (!(cin >> R >> C)) return 0;

    vector<string> grid(R);
    int cur_r = 0, cur_c = 0;
    for (int i = 0; i < R; i++) {
        cin >> grid[i];
        for (int j = 0; j < C; j++) {
            if (grid[i][j] == 'S') {
                cur_r = i;
                cur_c = j;
            }
        }
    }

    string commands;
    cin >> commands;

    vector<char> history;
    

    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    
    auto get_dir_idx = [](char cmd) {
        if (cmd == 'U') return 0;
        if (cmd == 'D') return 1;
        if (cmd == 'L') return 2;
        return 3; // 'R'
    };

    for (char cmd : commands) {
        if (cmd == 'Z') {
            if (!history.empty()) {
                char last_cmd = history.back();
                history.pop_back();
                
                int idx = get_dir_idx(last_cmd);
                // 反向退回
                cur_r -= dr[idx];
                cur_c -= dc[idx];
            }
        } else {
           
            int idx = get_dir_idx(cmd);
            int nr = cur_r + dr[idx];
            int nc = cur_c + dc[idx];

            if (nr >= 0 && nr < R && nc >= 0 && nc < C && grid[nr][nc] != '#') {
                // 有效移动，更新位置并记录到栈中
                cur_r = nr;
                cur_c = nc;
                history.push_back(cmd);
            }
            // 如果是出界或遇到 '#', 则什么都不做（原地不动，不记录）
        }
    }

    // 输出指令执行完后的位置
    cout << cur_r + 1 << " " << cur_c + 1 << "\n";

    // 2. 原路返回：清空栈，生成相反的动作
    string return_path = "";
    while (!history.empty()) {
        char last_cmd = history.back();
        history.pop_back();
        
        // 根据来时的方向，生成返回的方向
        if (last_cmd == 'U') return_path += 'D';
        else if (last_cmd == 'D') return_path += 'U';
        else if (last_cmd == 'L') return_path += 'R';
        else if (last_cmd == 'R') return_path += 'L';
    }

    // 输出返回路径
    if (return_path.empty()) {
        cout << "STAY\n";
    } else {
        cout << return_path << "\n";
    }

    return 0;
}