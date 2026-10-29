#include <iostream>
#include <vector>
#include <algorithm> // 必须包含，为了使用 upper_bound

using namespace std;

int main() {
    // 加速输入输出
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    //计算前缀和
    vector<long long> pre(n + 1, 0);
    for (int i = 1; i <= n; i++) {
        cin >> pre[i];
        pre[i] += pre[i - 1]; 
    }

    int q;
    cin >> q;
    
    // 处理每次询问
    while (q--) {
        int l;
        long long E;
        cin >> l >> E;

        long long target = pre[l - 1] + E;

        int r = upper_bound(pre.begin() + l, pre.end(), target) - pre.begin() - 1;

        if (r < l) {
            cout << 0 << "\n";
        } else {
            cout << r - l + 1 << "\n";
        }
    }

    return 0;
}