#include <bits/stdc++.h>

using namespace std;

vector<string> split_string(string);

// Complete the kangaroo function below.
string kangaroo(int x1, int v1, int x2, int v2) {
    
    long temp = 0;
    string abc="";
    long s1 = x1;
    long s2 = x2;
    long temp1 = x1-x2;
    if (v1==v2 && temp1!=0){
        abc.append("NO");
        return abc;
    }
    while(1){
        temp = s1-s2;
        if (temp==0){
            abc.append("YES");
            break;
        }
        else if ((temp1*temp)<0){
            abc.append("NO");
            break;
        }
        else if (abs(temp1)<abs(temp)){
            abc.append("NO");
            break;
        }

        s1+=(v1);
        s2+=(v2);
    }
    return abc;

}

int main()
{
    ofstream fout(getenv("OUTPUT_PATH"));

    string x1V1X2V2_temp;
    getline(cin, x1V1X2V2_temp);

    vector<string> x1V1X2V2 = split_string(x1V1X2V2_temp);

    int x1 = stoi(x1V1X2V2[0]);

    int v1 = stoi(x1V1X2V2[1]);

    int x2 = stoi(x1V1X2V2[2]);

    int v2 = stoi(x1V1X2V2[3]);

    string result = kangaroo(x1, v1, x2, v2);

    fout << result << "\n";

    fout.close();

    return 0;
}

vector<string> split_string(string input_string) {
    string::iterator new_end = unique(input_string.begin(), input_string.end(), [] (const char &x, const char &y) {
        return x == y and x == ' ';
    });

    input_string.erase(new_end, input_string.end());

    while (input_string[input_string.length() - 1] == ' ') {
        input_string.pop_back();
    }

    vector<string> splits;
    char delimiter = ' ';

    size_t i = 0;
    size_t pos = input_string.find(delimiter);

    while (pos != string::npos) {
        splits.push_back(input_string.substr(i, pos - i));

        i = pos + 1;
        pos = input_string.find(delimiter, i);
    }

    splits.push_back(input_string.substr(i, min(pos, input_string.length()) - i + 1));

    return splits;
}