#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>

using namespace std;

//上，下，左，右
int dx[4] = {-1, 1, 0, 0};
int dy[4] = {0, 0, -1, 1};
char dir_char[4] = {'U', 'D', 'L', 'R'};

int main() {

    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int R, C;
    if (!(cin >> R >> C)) return 0;

    vector<string> grid(R);
    int start_x = -1, start_y = -1;
    int target_x = -1, target_y = -1;

    for (int i = 0; i < R; i++) {
        cin >> grid[i];
        for (int j = 0; j < C; j++) {
            if (grid[i][j] == 'S') {
                start_x = i;
                start_y = j;
            } else if (grid[i][j] == 'T') {
                target_x = i;
                target_y = j;
            }
        }
    }

    // dist 数组记录最短距离
    // pre_dir 数组记录到达该点的方向，方便回溯
    vector<vector<int>> dist(R, vector<int>(C, -1));
    vector<vector<int>> pre_dir(R, vector<int>(C, -1));

    queue<pair<int, int>> q;
    q.push({start_x, start_y});
    dist[start_x][start_y] = 0;

    bool found = false;

    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();

        if (x == target_x && y == target_y) {
            found = true;
            break;
        }

        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            // 检查边界、障碍物和是否已访问
            if (nx >= 0 && nx < R && ny >= 0 && ny < C && 
                grid[nx][ny] != '#' && dist[nx][ny] == -1) {
                
                dist[nx][ny] = dist[x][y] + 1;
                pre_dir[nx][ny] = i; // 记录是从哪个方向过来的
                q.push({nx, ny});
            }
        }
    }

    if (!found) {
        cout << -1 << "\n";
    } else {
        cout << dist[target_x][target_y] << "\n";

        // 从终点回溯到起点
        string path = "";
        int cx = target_x;
        int cy = target_y;

        while (cx != start_x || cy != start_y) {
            int d = pre_dir[cx][cy];
            path += dir_char[d];
            
            // 逆向推导前一个坐标
            cx -= dx[d];
            cy -= dy[d];
        }

        reverse(path.begin(), path.end());
        cout << path << "\n";
    }

    return 0;
}