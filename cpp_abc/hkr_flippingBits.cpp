#include <bits/stdc++.h>

using namespace std;

// Complete the flippingBits function below.
long flippingBits(long n) {
    unsigned int result =0;
    for (int i=0; i<32; i++){
        // result +=((~(n>>i)&(1))<<i) ;
        // result +=((!(n&(0x00000001<<i)))<<i);
		result = (~n);
    }
    return result;
}

int main()
{
    ofstream fout(getenv("OUTPUT_PATH"));

    int q;
	// cout << "Enter number of sample: ";
    cin >> q;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

	// cout << "Enter Sample: ";
    for (int q_itr = 0; q_itr < q; q_itr++) {
        long n;
        cin >> n;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        long result = flippingBits(n);

        cout << result << "\n";
    }

    fout.close();

    return 0;
}
