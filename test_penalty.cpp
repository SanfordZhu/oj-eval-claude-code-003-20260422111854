#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

int main() {
    // Test penalty calculation
    // Team solves problem A at time 100 with 2 wrong attempts
    // Penalty = 20 * 2 + 100 = 140

    int wrong_attempts = 2;
    int solve_time = 100;
    int penalty = 20 * wrong_attempts + solve_time;

    cout << "Penalty calculation test:\n";
    cout << "Wrong attempts: " << wrong_attempts << "\n";
    cout << "Solve time: " << solve_time << "\n";
    cout << "Penalty: 20 * " << wrong_attempts << " + " << solve_time << " = " << penalty << "\n";

    return 0;
}