#include <vector>
#include <cstdio>
#include <cstring>
#include <iostream>

using namespace std;
class Potato {
    public:
        int hops_remain;
        int hops_total;
        int trace[512];
        Potato(int n) {
            hops_remain = n;
            hops_total = n;
            memset(trace, 0, sizeof(trace));
        }

        //Print the trace of the current potato
        void printTrace() {
            cout << "Trace of potato:" << endl;
            for (int i = 0; i < hops_total; i++) {
                cout << trace[i];
                if (i == hops_total - 1) {
                    cout << endl;
                }
                cout << ",";
            }
        }
};

