#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {

    ios_base::sync_with_stdio(false);
    cin.tie(0);

    int R, C, mode;
    cin >> R >> C >> mode;

    // 使用一维字符串数组存地图，最基础的做法
    vector<string> grid(R);
    int sx, sy, tx, ty;
    
    // 读入地图并找起点终点
    for (int i = 0; i < R; i++) {
        cin >> grid[i];
        for (int j = 0; j < C; j++) {
            if (grid[i][j] == 'S') {
                sx = i; sy = j;
            }
            if (grid[i][j] == 'T') {
                tx = i; ty = j;
            }
        }
    }

    // 确定方向：为了不用 vector 存方向，我们直接开两个固定大小的数组
    int dx[4] = {-1, 1, 0, 0};
    int dy[4] = {0, 0, -1, 1};
    int dir_count = 4; // 默认4个方向
    
    // 如果 mode == 1，只能向下、向右，我们改变方向和数量
    if (mode == 1) {
        dx[0] = 1;  dy[0] = 0; // 下
        dx[1] = 0;  dy[1] = 1; // 右
        dir_count = 2;         // 只有2个方向
    }

    // dist 数组记录到达每个点的最小代价
    vector<vector<int>> dist(R, vector<int>(C, 1e10));
    
    // visited 数组记录这个格子是否已经确定了最小代价
    vector<vector<bool>> visited(R, vector<bool>(C, false));

    // 起点代价为 0
    dist[sx][sy] = 0;

    // 3. 朴素 Dijkstra 核心（不用队列，纯循环）
    for (int k = 0; k < R * C; k++) {
        
        // 寻找当前还没访问过、且代价最小的点
        int cx = -1, cy = -1;
        int min_d = 1e10;
        
        for (int i = 0; i < R; i++) {
            for (int j = 0; j < C; j++) {
                if (!visited[i][j] && dist[i][j] < min_d) {
                    min_d = dist[i][j];
                    cx = i;
                    cy = j;
                }
            }
        }

        // 如果找不到可以延伸的点了,退出循环
        if (cx == -1 || (cx == tx && cy == ty)) {
            break;
        }

        // 标记这个点已经被确定（以后再也不看它了）
        visited[cx][cy] = true;

        // 更新四周的邻居
        for (int i = 0; i < dir_count; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            // 检查边界和障碍物
            if (nx >= 0 && nx < R && ny >= 0 && ny < C && grid[nx][ny] != '#') {
                
                // 计算走进去的代价
                char c = grid[nx][ny];
                int step_cost = 1; // 默认是 '.', 'S', 'T' 代价为 1
                if (c == '~') {
                    step_cost = 5;
                } else if (c >= '1' && c <= '9') {
                    step_cost = c - '0'; // 字符数字转整数
                }

                // 如果通过当前点 (cx,cy) 走到邻居 (nx,ny) 的总代价，比邻居原来记录的更小
                if (dist[cx][cy] + step_cost < dist[nx][ny]) {
                    dist[nx][ny] = dist[cx][cy] + step_cost; // 更新邻居的代价
                }
            }
        }
    }

    // 输出结果
    if (dist[tx][ty] == 1e10) {
        cout << -1 << "\n"; // 依然是无穷大，说明走不到
    } else {
        cout << dist[tx][ty] << "\n";
    }

    return 0;
}