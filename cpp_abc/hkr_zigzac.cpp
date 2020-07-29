#include <bits/stdc++.h>
using namespace std;

void findZigZagSequence(vector < int > a, int n){
    sort(a.begin(), a.end());
    int mid = (n + 1)/2-1;
    swap(a[mid], a[n-1]);

    int st = mid + 1;
    int ed = n - 2;
    while(st <= ed){
        swap(a[st], a[ed]);
        st = st + 1;
        ed = ed - 1;
    }
    for(int i = 0; i < n; i++){
        if(i > 0) cout << " ";
        cout << a[i];
    }
    cout << endl;
}

int main() {
    int n, x;
    int test_cases;
    // cin >> test_cases;
	test_cases =1;
    for(int cs = 1; cs <= test_cases; cs++){
		cout << "Enter number of array:	";
        cin >> n;
		cout << "Enter element of array:" <<endl;
        vector < int > a;
        for(int i = 0; i < n; i++){
            cin >> x;
            a.push_back(x);
        }
        findZigZagSequence(a, n);
    }
}



