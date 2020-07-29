#include <bits/stdc++.h>

using namespace std;

string ltrim(const string &);
string rtrim(const string &);
vector<string> split(const string &);

/*
 * Complete the 'getTotalX' function below.
 *
 * The function is expected to return an INTEGER.
 * The function accepts following parameters:
 *  1. INTEGER_ARRAY a
 *  2. INTEGER_ARRAY b
 */

int gcd(int a, int b)  
{  
    if (a == 0) 
        return b;  
    return gcd(b % a, a);  
}  
  
// Function to return LCM of two numbers  
int lcm(int a, int b)  
{  
    return (a*b)/gcd(a, b);  
}  
int getTotalX(vector<int> a, vector<int> b) {
    int lcma=a[0];
    for (vector<int>::iterator it = (a.begin()+1); it!=a.end(); ++it){
        // int hcf = lcma;
        // int temp = *it;
        // int lcm;
    
        // while(hcf != temp)
        // {
        //     if(hcf > temp)
        //         hcf -= temp;
        //     else
        //         temp -= hcf;
        // }

        // lcm = (lcma * (*it)) / hcf;
        // lcma = lcm;

		// int hcf = lcma;
        int temp = *it;
		lcma = lcm(temp, lcma);
    }

    int hcfb = b[0];
    for (vector<int>::iterator it = (b.begin()+1); it!=b.end(); ++it){
        // int hcf = hcfb;
        int temp = *it;
    
        // while(hcf != temp)
        // {
        //     if(hcf > temp)
        //         hcf -= temp;
        //     else
        //         temp -= hcf;
        // }
		// hcfb = hcf;
		hcfb = gcd(temp, hcfb);
    }
		int count = 0;
		int i = lcma;
		while (i<=hcfb){
			if (hcfb%i==0) count++;
			i+=lcma;
		}
		return count;
	
}

int main()
{
    ofstream fout(getenv("OUTPUT_PATH"));

    // string first_multiple_input_temp;
    // getline(cin, first_multiple_input_temp);

    // vector<string> first_multiple_input = split(rtrim(first_multiple_input_temp));

    // int n = stoi(first_multiple_input[0]);

    // int m = stoi(first_multiple_input[1]);

    // string arr_temp_temp;
    // getline(cin, arr_temp_temp);

    // vector<string> arr_temp = split(rtrim(arr_temp_temp));

    // vector<int> arr(n);

    // for (int i = 0; i < n; i++) {
    //     int arr_item = stoi(arr_temp[i]);

    //     arr[i] = arr_item;
    // }

    // string brr_temp_temp;
    // getline(cin, brr_temp_temp);

    // vector<string> brr_temp = split(rtrim(brr_temp_temp));

    // vector<int> brr(m);

    // for (int i = 0; i < m; i++) {
    //     int brr_item = stoi(brr_temp[i]);

    //     brr[i] = brr_item;
    // }

vector<int> arr{2,4};
vector<int> brr{16,32,96};

    int total = getTotalX(arr, brr);
	
    cout << total << "\n";

    fout.close();

    return 0;
}

string ltrim(const string &str) {
    string s(str);

    s.erase(
        s.begin(),
        find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace)))
    );

    return s;
}

string rtrim(const string &str) {
    string s(str);

    s.erase(
        find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(),
        s.end()
    );

    return s;
}

vector<string> split(const string &str) {
    vector<string> tokens;

    string::size_type start = 0;
    string::size_type end = 0;

    while ((end = str.find(" ", start)) != string::npos) {
        tokens.push_back(str.substr(start, end - start));

        start = end + 1;
    }

    tokens.push_back(str.substr(start));

    return tokens;
}
