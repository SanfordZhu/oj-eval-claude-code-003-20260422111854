#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <climits>
#include <cstring>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;

    Submission(string p, string s, int t) : problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    int incorrect_attempts = 0;
    int solve_time = 0;
    bool solved = false;
    int frozen_submissions = 0;
    bool frozen = false;

    string getDisplayString() const {
        if (frozen) {
            return (incorrect_attempts == 0 ? "0" : to_string(incorrect_attempts)) + "/" + to_string(frozen_submissions);
        } else if (solved) {
            return incorrect_attempts == 0 ? "+" : "+" + to_string(incorrect_attempts);
        } else {
            return incorrect_attempts == 0 ? "." : "-" + to_string(incorrect_attempts);
        }
    }
};

struct Team {
    string name;
    int solved_count = 0;
    int total_penalty = 0;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;
    int last_flush_rank = 0;

    // For tie-breaking: store solve times in descending order
    vector<int> solve_times;

    void updateSolveTimes() {
        solve_times.clear();
        for (auto& [problem, status] : problems) {
            if (status.solved) {
                solve_times.push_back(status.solve_time);
            }
        }
        sort(solve_times.rbegin(), solve_times.rend());
    }
};

struct TeamComparator {
    bool operator()(const Team* a, const Team* b) const {
        if (a->solved_count != b->solved_count) {
            return a->solved_count > b->solved_count;
        }
        if (a->total_penalty != b->total_penalty) {
            return a->total_penalty < b->total_penalty;
        }

        // Compare solve times in descending order
        int min_size = min(a->solve_times.size(), b->solve_times.size());
        for (int i = 0; i < min_size; i++) {
            if (a->solve_times[i] != b->solve_times[i]) {
                return a->solve_times[i] < b->solve_times[i];
            }
        }
        if (a->solve_times.size() != b->solve_times.size()) {
            return a->solve_times.size() > b->solve_times.size();
        }

        // Lexicographic comparison of team names
        return a->name < b->name;
    }
};

class ICPCManager {
private:
    map<string, Team> teams;
    bool competition_started = false;
    bool competition_ended = false;
    int duration_time = 0;
    int problem_count = 0;
    bool frozen = false;
    int freeze_time = 0;

    // Current ranking (updated on flush)
    vector<Team*> current_ranking;

    // For scroll operation
    vector<pair<string, string>> scroll_changes; // team1, team2 pairs where ranking changed

public:
    void addTeam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.count(team_name)) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        teams[team_name] = Team{team_name};
        cout << "[Info]Add successfully.\n";
    }

    void startCompetition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        duration_time = duration;
        problem_count = problems;
        competition_started = true;

        // Initialize problem status for all teams
        for (auto& [name, team] : teams) {
            for (int i = 0; i < problem_count; i++) {
                char prob = 'A' + i;
                team.problems[string(1, prob)] = ProblemStatus{};
            }
        }

        cout << "[Info]Competition starts.\n";
    }

    void submit(const string& problem, const string& team_name, const string& status, int time) {
        if (!competition_started || competition_ended) return;

        Team& team = teams[team_name];
        team.submissions.emplace_back(problem, status, time);

        ProblemStatus& prob_status = team.problems[problem];

        if (frozen && !prob_status.solved) {
            // During freeze, only track frozen submissions
            prob_status.frozen_submissions++;
            prob_status.frozen = true;
        } else if (!prob_status.solved) {
            // Before freeze or after scroll
            if (status == "Accepted") {
                prob_status.solved = true;
                prob_status.solve_time = time;
                team.solved_count++;
                team.total_penalty += 20 * prob_status.incorrect_attempts + time;
            } else {
                prob_status.incorrect_attempts++;
            }
        }
    }

    void flushScoreboard() {
        if (!competition_started) return;

        // Update all teams' solve times
        for (auto& [name, team] : teams) {
            team.updateSolveTimes();
        }

        // Build current ranking
        current_ranking.clear();
        for (auto& [name, team] : teams) {
            current_ranking.push_back(&team);
        }
        sort(current_ranking.begin(), current_ranking.end(), TeamComparator{});

        // Update last flush rank for each team
        for (int i = 0; i < current_ranking.size(); i++) {
            current_ranking[i]->last_flush_rank = i + 1;
        }

        cout << "[Info]Flush scoreboard.\n";

        // Always print the scoreboard after flushing
        printScoreboard();
    }

    void freezeScoreboard() {
        if (!competition_started || competition_ended) return;
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        frozen = true;
        // Get the last submission time from all teams
        freeze_time = 0;
        for (auto& [name, team] : teams) {
            if (!team.submissions.empty()) {
                freeze_time = max(freeze_time, team.submissions.back().time);
            }
        }
        cout << "[Info]Freeze scoreboard.\n";
    }

    void scrollScoreboard() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // First flush the scoreboard
        flushScoreboard();

        // Print scoreboard before scrolling
        printScoreboard();

        // Process scroll
        scroll_changes.clear();

        while (true) {
            // Find team with lowest rank that has frozen problems
            Team* lowest_frozen_team = nullptr;
            string lowest_frozen_problem;
            int lowest_rank = INT_MAX;

            for (auto& team_ptr : current_ranking) {
                if (team_ptr->last_flush_rank >= lowest_rank) continue;

                for (auto& [prob, status] : team_ptr->problems) {
                    if (status.frozen) {
                        if (team_ptr->last_flush_rank < lowest_rank) {
                            lowest_rank = team_ptr->last_flush_rank;
                            lowest_frozen_team = team_ptr;
                            lowest_frozen_problem = prob;
                        }
                        break;
                    }
                }
            }

            if (!lowest_frozen_team) break;

            // Unfreeze the problem
            ProblemStatus& prob_status = lowest_frozen_team->problems[lowest_frozen_problem];
            prob_status.frozen = false;

            // Process submissions during freeze for this problem
            int last_accepted_time = -1;
            int accepted_count = 0;

            for (const auto& sub : lowest_frozen_team->submissions) {
                if (sub.problem == lowest_frozen_problem && sub.time > freeze_time) {
                    if (sub.status == "Accepted" && last_accepted_time == -1) {
                        last_accepted_time = sub.time;
                        accepted_count++;
                    } else if (sub.status != "Accepted" && last_accepted_time == -1) {
                        prob_status.incorrect_attempts++;
                    }
                }
            }

            if (last_accepted_time != -1 && !prob_status.solved) {
                prob_status.solved = true;
                prob_status.solve_time = last_accepted_time;
                lowest_frozen_team->solved_count++;
                lowest_frozen_team->total_penalty += 20 * prob_status.incorrect_attempts + last_accepted_time;
            }

            // Recalculate ranking
            vector<Team*> old_ranking = current_ranking;

            // Update solve times
            for (auto& [name, team] : teams) {
                team.updateSolveTimes();
            }

            // Sort again
            sort(current_ranking.begin(), current_ranking.end(), TeamComparator{});

            // Update ranks and check for changes
            for (int i = 0; i < current_ranking.size(); i++) {
                current_ranking[i]->last_flush_rank = i + 1;
            }

            // Find ranking changes
            map<Team*, int> old_positions;
            for (int i = 0; i < old_ranking.size(); i++) {
                old_positions[old_ranking[i]] = i;
            }

            for (int i = 0; i < current_ranking.size(); i++) {
                Team* team = current_ranking[i];
                int old_pos = old_positions[team];
                if (old_pos != i) { // Position changed
                    if (i < old_pos) { // Ranking improved
                        // Find the team that was at this position before
                        Team* displaced_team = old_ranking[i];
                        cout << team->name << " "
                             << displaced_team->name << " "
                             << team->solved_count << " "
                             << team->total_penalty << "\n";
                    }
                }
            }
        }

        frozen = false;

        // Print scoreboard after scrolling
        printScoreboard();
    }

    void queryRanking(const string& team_name) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        Team& team = teams[team_name];
        // If never flushed, calculate rank based on alphabetical order
        if (team.last_flush_rank == 0 && competition_started) {
            int rank = 1;
            for (auto& [name, other_team] : teams) {
                if (name < team_name) rank++;
            }
            cout << team_name << " NOW AT RANKING " << rank << "\n";
        } else {
            cout << team_name << " NOW AT RANKING " << team.last_flush_rank << "\n";
        }
    }

    void querySubmission(const string& team_name, const string& problem, const string& status) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        Team& team = teams[team_name];
        Submission* last_match = nullptr;

        for (auto it = team.submissions.rbegin(); it != team.submissions.rend(); ++it) {
            bool prob_match = (problem == "ALL" || it->problem == problem);
            bool status_match = (status == "ALL" || it->status == status);

            if (prob_match && status_match) {
                last_match = &(*it);
                break;
            }
        }

        if (!last_match) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << team_name << " " << last_match->problem << " "
                 << last_match->status << " " << last_match->time << "\n";
        }
    }

    void endCompetition() {
        competition_ended = true;
        cout << "[Info]Competition ends.\n";
    }

private:
    void printScoreboard() {
        for (Team* team : current_ranking) {
            cout << team->name << " " << team->last_flush_rank << " "
                 << team->solved_count << " " << team->total_penalty;

            for (int i = 0; i < problem_count; i++) {
                char prob = 'A' + i;
                cout << " " << team->problems[string(1, prob)].getDisplayString();
            }
            cout << "\n";
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPCManager manager;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string command;
        iss >> command;

        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            manager.addTeam(team_name);
        } else if (command == "START") {
            string dummy;
            int duration, problems;
            iss >> dummy >> duration >> dummy >> problems;
            manager.startCompetition(duration, problems);
        } else if (command == "SUBMIT") {
            string problem, dummy1, team, dummy2, status, dummy3;
            int time;
            iss >> problem >> dummy1 >> team >> dummy2 >> status >> dummy3 >> time;
            manager.submit(problem, team, status, time);
        } else if (command == "FLUSH") {
            manager.flushScoreboard();
        } else if (command == "FREEZE") {
            manager.freezeScoreboard();
        } else if (command == "SCROLL") {
            manager.scrollScoreboard();
        } else if (command == "QUERY_RANKING") {
            string team;
            iss >> team;
            manager.queryRanking(team);
        } else if (command == "QUERY_SUBMISSION") {
            string team, dummy, prob_status;
            iss >> team >> dummy >> prob_status;

            // Parse WHERE clause
            string problem = "ALL";
            string status = "ALL";

            size_t prob_pos = prob_status.find("PROBLEM=");
            size_t status_pos = prob_status.find("STATUS=");

            if (prob_pos != string::npos) {
                size_t start = prob_pos + 8;
                size_t end = prob_status.find(" AND", start);
                if (end == string::npos) end = prob_status.length();
                problem = prob_status.substr(start, end - start);
            }

            if (status_pos != string::npos) {
                size_t start = status_pos + 7;
                status = prob_status.substr(start);
            }

            manager.querySubmission(team, problem, status);
        } else if (command == "END") {
            manager.endCompetition();
            break;
        }
    }

    return 0;
}