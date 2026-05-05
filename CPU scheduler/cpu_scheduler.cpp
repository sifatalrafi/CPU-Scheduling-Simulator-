/*
 ╔══════════════════════════════════════════════════════╗
 ║    CPU SCHEDULING ALGORITHM SIMULATOR  v2.0          ║
 ║    CSE323 — Operating Systems Project                ║
 ╚══════════════════════════════════════════════════════╝
*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <queue>
#include <climits>
#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <cmath>

using namespace std;

// ══════════════════════════════════════════
//  ANSI COLOR CODES
// ══════════════════════════════════════════
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"
#define BG_BLUE     "\033[44m"
#define BG_CYAN     "\033[46m"
#define BG_GREEN    "\033[42m"
#define BG_RED      "\033[41m"
#define BG_MAGENTA  "\033[45m"
#define BG_YELLOW   "\033[43m"

// Colors for processes (cycles through)
const string PROC_COLORS[] = {
    "\033[42m", "\033[44m", "\033[45m", "\033[43m",
    "\033[46m", "\033[41m", "\033[102m", "\033[104m"
};
const int NUM_COLORS = 8;

// ══════════════════════════════════════════
//  PROCESS STATES
// ══════════════════════════════════════════
enum ProcessState { NEW, READY, RUNNING, WAITING, TERMINATED };

string stateStr(ProcessState s) {
    switch(s) {
        case NEW:        return GREEN  + string("NEW")        + RESET;
        case READY:      return YELLOW + string("READY")      + RESET;
        case RUNNING:    return CYAN   + string("RUNNING")    + RESET;
        case WAITING:    return MAGENTA+ string("WAITING")    + RESET;
        case TERMINATED: return DIM    + string("TERMINATED") + RESET;
    }
    return "";
}

// ══════════════════════════════════════════
//  PROCESS STRUCTURE
// ══════════════════════════════════════════
struct Process {
    int id;
    string name;
    int arrivalTime;
    int burstTime;
    int priority;
    int queueLevel;       // For Multilevel Queue
    int remainingTime;
    int completionTime;
    int waitingTime;
    int turnaroundTime;
    int responseTime;     // First time it got CPU
    bool firstRun;
    ProcessState state;
    int ageCounter;       // For aging in Priority Scheduling
    vector<pair<int,int>> timeline; // {start, end} execution slices
};

// ══════════════════════════════════════════
//  GLOBAL SETTINGS
// ══════════════════════════════════════════
struct Settings {
    bool contextSwitchEnabled = false;
    int contextSwitchTime = 1;
    bool agingEnabled = false;
    int agingThreshold = 5;  // boost priority after this many waiting units
    bool saveReport = false;
    string reportFile = "scheduling_report.txt";
};

Settings globalSettings;

// ══════════════════════════════════════════
//  UI HELPERS
// ══════════════════════════════════════════
void clearScreen() {
    cout << "\033[2J\033[1;1H";
}

void printHeader() {
    cout << BOLD << CYAN;
    cout << "\n╔═════════════════════════════════════════════════════════╗\n";
    cout << "║                                                          ║\n";
    cout << "║      ██████╗██████╗ ██╗   ██╗    ███████╗██╗███╗   ███╗  ║\n";
    cout << "║     ██╔════╝██╔══██╗██║   ██║    ██╔════╝██║████╗ ████║  ║\n";
    cout << "║     ██║     ██████╔╝██║   ██║    ███████╗██║██╔████╔██║  ║\n";
    cout << "║     ██║     ██╔═══╝ ██║   ██║    ╚════██║██║██║╚██╔╝██║  ║\n";
    cout << "║     ╚██████╗██║     ╚██████╔╝    ███████║██║██║ ╚═╝ ██║  ║\n";
    cout << "║      ╚═════╝╚═╝      ╚═════╝     ╚══════╝╚═╝╚═╝     ╚═╝  ║\n";
    cout << "║                                                          ║\n";
    cout << "║         SCHEDULING ALGORITHM SIMULATOR  v2.0             ║\n";
    cout << "║              CSE323 — Operating Systems                  ║\n";
    cout << "╚══════════════════════════════════════════════════════════╝\n";
    cout << RESET;
}

void printSectionHeader(const string& title) {
    int width = 58;
    int pad = (width - (int)title.size()) / 2;
    cout << BOLD << BLUE;
    cout << "\n╔";
    for(int i=0;i<width;i++) cout << "═";
    cout << "╗\n║";
    for(int i=0;i<pad;i++) cout << " ";
    cout << YELLOW << title << BLUE;
    for(int i=0;i<width-(int)title.size()-pad;i++) cout << " ";
    cout << "║\n╚";
    for(int i=0;i<width;i++) cout << "═";
    cout << "╝\n" << RESET;
}

void printDivider(bool thick = false, int width = 60) {
    cout << DIM;
    string ch = thick ? "=" : "-";
    for(int i=0;i<width;i++) cout << ch;
    cout << "\n" << RESET;
}

void printBar(int value, int maxVal, int barWidth = 20, string color = GREEN) {
    int filled = (maxVal > 0) ? (value * barWidth / maxVal) : 0;
    cout << color << "[";
    for(int i=0;i<barWidth;i++)
        cout << (i < filled ? "█" : "░");
    cout << "] " << RESET << value;
}

// ══════════════════════════════════════════
//  INPUT
// ══════════════════════════════════════════
vector<Process> getProcesses(bool needPriority, bool needQueue = false) {
    int n;
    cout << BOLD << "\n  ┌─ Process Input\n" << RESET;
    cout << "  │  Enter number of processes: ";
    cin >> n;
    cout << "  └─\n\n";

    vector<Process> procs(n);
    int maxBurst = 1;

    for (int i = 0; i < n; i++) {
        procs[i].id = i + 1;
        procs[i].name = "P" + to_string(i + 1);
        procs[i].firstRun = true;
        procs[i].responseTime = -1;
        procs[i].ageCounter = 0;
        procs[i].state = NEW;

        cout << BOLD << CYAN << "  ┌─ Process " << procs[i].name << RESET << "\n";
        cout << "  │  Arrival Time              : "; cin >> procs[i].arrivalTime;
        cout << "  │  Burst Time                : "; cin >> procs[i].burstTime;
        maxBurst = max(maxBurst, procs[i].burstTime);
        if (needPriority) {
            cout << "  │  Priority (1=highest)      : "; cin >> procs[i].priority;
        } else {
            procs[i].priority = 0;
        }
        if (needQueue) {
            cout << "  │  Queue Level (0=highest,1,2): "; cin >> procs[i].queueLevel;
        } else {
            procs[i].queueLevel = 0;
        }
        procs[i].remainingTime = procs[i].burstTime;
        procs[i].completionTime = 0;
        procs[i].waitingTime = 0;
        procs[i].turnaroundTime = 0;
        cout << "  └─\n\n";
    }

    // Show entered processes visually
    cout << BOLD << "\n  Processes entered:\n\n" << RESET;
    cout << "  " << BOLD;
    cout << left << setw(6) << "PID" << setw(10) << "Arrival" << setw(10) << "Burst";
    if(needPriority) cout << setw(10) << "Priority";
    if(needQueue)    cout << setw(10) << "Queue";
    cout << "Burst Visualization\n" << RESET;
    printDivider(false, 60);

    for (auto& p : procs) {
        cout << "  " << left
             << setw(6) << p.name
             << setw(10) << p.arrivalTime
             << setw(10) << p.burstTime;
        if(needPriority) cout << setw(10) << p.priority;
        if(needQueue)    cout << setw(10) << p.queueLevel;
        printBar(p.burstTime, maxBurst, 20, PROC_COLORS[p.id % NUM_COLORS] + WHITE);
        cout << "\n";
    }
    printDivider();

    return procs;
}

// ══════════════════════════════════════════
//  GANTT CHART
// ══════════════════════════════════════════
void displayGantt(const vector<pair<int,int>>& gantt) {
    if (gantt.empty()) return;

    cout << BOLD << "\n  ┌─ Gantt Chart\n" << RESET;

    // Top bar
    cout << "  │\n  │  ";
    for (const auto& g : gantt) {
        string color = (g.first == -1) ? DIM : (PROC_COLORS[(g.first-1) % NUM_COLORS] + WHITE);
        cout << color << "┌──────┐" << RESET;
    }
    cout << "\n  │  ";

    // Process names
    for (const auto& g : gantt) {
        string color = (g.first == -1) ? DIM : (PROC_COLORS[(g.first-1) % NUM_COLORS] + WHITE + BOLD);
        string label = (g.first == -1) ? " IDLE " : ("  P" + to_string(g.first) + "  ");
        if (label.size() < 6) label = " " + label;
        cout << color << "│" << label.substr(0,6) << "│" << RESET;
    }
    cout << "\n  │  ";

    // Bottom bar
    for (const auto& g : gantt) {
        string color = (g.first == -1) ? DIM : (PROC_COLORS[(g.first-1) % NUM_COLORS] + WHITE);
        cout << color << "└──────┘" << RESET;
    }
    cout << "\n  │\n  │  ";

    // Time stamps
    int startTime = 0;
    // find initial time from first gantt block
    // We'll reconstruct: track cumulative
    // gantt stores {pid, endTime}
    // so start of first = need to find
    // We store start separately — let's compute from gantt sequence
    // Actually gantt[i].second is endTime, so start of gantt[0] = 0 or first arrival
    // For simplicity, print end times only
    cout << YELLOW;
    // print 0 at start
    cout << "0";
    for (const auto& g : gantt) {
        cout << setw(7) << g.second;
    }
    cout << RESET << "\n  └─\n";
}

// Overload with start time info
void displayGanttFull(const vector<tuple<int,int,int>>& gantt) {
    // {pid, startTime, endTime}
    if (gantt.empty()) return;

    cout << BOLD << "\n  ┌─ Gantt Chart (Detailed)\n" << RESET;

    cout << "  │\n  │  ";
    for (const auto& g : gantt) {
        int pid = get<0>(g);
        string color = (pid == -1) ? DIM : (PROC_COLORS[(pid-1) % NUM_COLORS] + WHITE);
        cout << color << "┌──────┐" << RESET;
    }
    cout << "\n  │  ";

    for (const auto& g : gantt) {
        int pid = get<0>(g);
        string color = (pid == -1) ? DIM : (PROC_COLORS[(pid-1) % NUM_COLORS] + WHITE + BOLD);
        string label = (pid == -1) ? " IDLE " : ("  P" + to_string(pid) + "  ");
        cout << color << "│" << label.substr(0,6) << "│" << RESET;
    }
    cout << "\n  │  ";

    for (const auto& g : gantt) {
        int pid = get<0>(g);
        string color = (pid == -1) ? DIM : (PROC_COLORS[(pid-1) % NUM_COLORS] + WHITE);
        cout << color << "└──────┘" << RESET;
    }
    cout << "\n  │\n  │  " << YELLOW;

    // Time
    cout << get<1>(gantt[0]);
    for (const auto& g : gantt)
        cout << setw(7) << get<2>(g);

    cout << RESET << "\n  └─\n";
}

// ══════════════════════════════════════════
//  RESULTS TABLE
// ══════════════════════════════════════════
void displayResults(vector<Process>& procs, const string& algoName, bool showResponse = true) {
    printSectionHeader("Results: " + algoName);

    double totalWT = 0, totalTAT = 0, totalRT = 0;
    int maxTAT = 0, maxWT = 0;

    for (auto& p : procs) {
        totalWT  += p.waitingTime;
        totalTAT += p.turnaroundTime;
        if (p.responseTime >= 0) totalRT += p.responseTime;
        maxTAT = max(maxTAT, p.turnaroundTime);
        maxWT  = max(maxWT,  p.waitingTime);
    }

    int n = procs.size();

    // Header
    cout << BOLD << "  " << left
         << setw(6)  << "PID"
         << setw(9)  << "Arrival"
         << setw(7)  << "Burst"
         << setw(8)  << "Finish"
         << setw(7)  << "TAT"
         << setw(7)  << "WT";
    if(showResponse) cout << setw(7) << "RT";
    cout << "  State\n" << RESET;
    printDivider(false, 62);

    for (auto& p : procs) {
        p.state = TERMINATED;
        cout << "  " << PROC_COLORS[(p.id-1) % NUM_COLORS] << WHITE << BOLD
             << setw(5) << p.name << RESET << " "
             << setw(9) << p.arrivalTime
             << setw(7) << p.burstTime
             << setw(8) << p.completionTime
             << GREEN  << setw(7) << p.turnaroundTime << RESET
             << YELLOW << setw(7) << p.waitingTime    << RESET;
        if(showResponse && p.responseTime >= 0)
            cout << CYAN << setw(7) << p.responseTime << RESET;
        else if(showResponse)
            cout << setw(7) << "N/A";
        cout << "  " << stateStr(p.state) << "\n";
    }

    printDivider(true, 62);

    // Stats
    cout << "\n  " << BOLD << "Performance Summary\n\n" << RESET;

    cout << "  " << CYAN  << "Average Waiting Time    : " << RESET
         << fixed << setprecision(2) << totalWT / n;
    cout << "  ";
    printBar((int)(totalWT/n), maxWT > 0 ? maxWT : 1, 16, YELLOW);
    cout << "\n";

    cout << "  " << GREEN << "Average Turnaround Time : " << RESET
         << fixed << setprecision(2) << totalTAT / n;
    cout << "  ";
    printBar((int)(totalTAT/n), maxTAT > 0 ? maxTAT : 1, 16, GREEN);
    cout << "\n";

    if(showResponse) {
        cout << "  " << BLUE << "Average Response Time   : " << RESET
             << fixed << setprecision(2) << totalRT / n << "\n";
    }

    // CPU Utilization
    int totalBurst = 0;
    for (auto& p : procs) totalBurst += p.burstTime;
    int totalTime = 0;
    for (auto& p : procs) totalTime = max(totalTime, p.completionTime);
    double cpuUtil = (totalTime > 0) ? (100.0 * totalBurst / totalTime) : 100.0;

    cout << "\n  " << MAGENTA << "CPU Utilization         : " << RESET
         << fixed << setprecision(1) << cpuUtil << "%";
    cout << "  ";
    printBar((int)cpuUtil, 100, 16, MAGENTA);
    cout << "\n";

    // Throughput
    cout << "  " << YELLOW << "Throughput              : " << RESET
         << fixed << setprecision(3) << (double)n / totalTime
         << " processes/unit time\n";

    printDivider();

    // Save to report if enabled
    if (globalSettings.saveReport) {
        ofstream f(globalSettings.reportFile, ios::app);
        f << "\n=== " << algoName << " ===\n";
        f << left << setw(6) << "PID" << setw(9) << "Arrival" << setw(7) << "Burst"
          << setw(8) << "Finish" << setw(7) << "TAT" << setw(7) << "WT\n";
        f << string(44, '-') << "\n";
        for (auto& p : procs) {
            f << setw(6) << p.name << setw(9) << p.arrivalTime << setw(7) << p.burstTime
              << setw(8) << p.completionTime << setw(7) << p.turnaroundTime << setw(7) << p.waitingTime << "\n";
        }
        f << "\nAvg WT: " << totalWT/n << "  Avg TAT: " << totalTAT/n
          << "  CPU Util: " << cpuUtil << "%\n";
        f.close();
        cout << GREEN << "  ✔ Results saved to: " << globalSettings.reportFile << "\n" << RESET;
    }
}

// ══════════════════════════════════════════
//  STARVATION CHECK
// ══════════════════════════════════════════
void checkStarvation(const vector<Process>& procs) {
    bool found = false;
    for (auto& p : procs) {
        if (p.waitingTime > 3 * p.burstTime) {
            if (!found) {
                cout << RED << BOLD << "\n  ⚠ STARVATION WARNING:\n" << RESET;
                found = true;
            }
            cout << RED << "    → " << p.name << " waited " << p.waitingTime
                 << " units (burst=" << p.burstTime << ") — possible starvation!\n" << RESET;
        }
    }
}

// ══════════════════════════════════════════
//  PROCESS TIMELINE (per process execution slices)
// ══════════════════════════════════════════
void displayTimeline(const vector<Process>& procs) {
    cout << BOLD << "\n  ┌─ Per-Process Execution Timeline\n  │\n" << RESET;
    for (auto& p : procs) {
        cout << "  │  " << PROC_COLORS[(p.id-1)%NUM_COLORS] << WHITE << BOLD
             << " " << p.name << " " << RESET << "  ";
        for (auto& slice : p.timeline) {
            cout << CYAN << "[" << slice.first << "→" << slice.second << "]" << RESET << " ";
        }
        cout << "\n";
    }
    cout << "  └─\n";
}

// ══════════════════════════════════════════
//  1. FCFS
// ══════════════════════════════════════════
void fcfs(vector<Process> procs) {
    sort(procs.begin(), procs.end(), [](const Process& a, const Process& b){
        return a.arrivalTime < b.arrivalTime;
    });

    vector<pair<int,int>> gantt;
    int currentTime = 0;
    int ctxSwitch = globalSettings.contextSwitchEnabled ? globalSettings.contextSwitchTime : 0;

    for (auto& p : procs) {
        if (currentTime < p.arrivalTime) {
            gantt.push_back({-1, p.arrivalTime});
            currentTime = p.arrivalTime;
        }
        p.responseTime = currentTime - p.arrivalTime;
        p.timeline.push_back({currentTime, currentTime + p.burstTime});
        currentTime += p.burstTime;
        p.completionTime  = currentTime;
        p.turnaroundTime  = p.completionTime - p.arrivalTime;
        p.waitingTime     = p.turnaroundTime - p.burstTime;
        gantt.push_back({p.id, currentTime});
        if (ctxSwitch > 0) { gantt.push_back({-1, currentTime + ctxSwitch}); currentTime += ctxSwitch; }
    }

    displayGantt(gantt);
    displayTimeline(procs);
    displayResults(procs, "First Come First Serve (FCFS)");
    checkStarvation(procs);
}

// ══════════════════════════════════════════
//  2. SJF Non-Preemptive
// ══════════════════════════════════════════
void sjfNonPreemptive(vector<Process> procs) {
    int n = procs.size();
    vector<bool> done(n, false);
    vector<pair<int,int>> gantt;
    int currentTime = 0, completed = 0;
    int ctxSwitch = globalSettings.contextSwitchEnabled ? globalSettings.contextSwitchTime : 0;

    while (completed < n) {
        int idx = -1, minBurst = INT_MAX;
        for (int i = 0; i < n; i++)
            if (!done[i] && procs[i].arrivalTime <= currentTime && procs[i].burstTime < minBurst) {
                minBurst = procs[i].burstTime; idx = i;
            }
        if (idx == -1) {
            int next = INT_MAX;
            for (int i=0;i<n;i++) if (!done[i]) next = min(next, procs[i].arrivalTime);
            gantt.push_back({-1, next}); currentTime = next; continue;
        }
        procs[idx].responseTime = currentTime - procs[idx].arrivalTime;
        procs[idx].timeline.push_back({currentTime, currentTime + procs[idx].burstTime});
        currentTime += procs[idx].burstTime;
        procs[idx].completionTime = currentTime;
        procs[idx].turnaroundTime = procs[idx].completionTime - procs[idx].arrivalTime;
        procs[idx].waitingTime    = procs[idx].turnaroundTime - procs[idx].burstTime;
        done[idx] = true; completed++;
        gantt.push_back({procs[idx].id, currentTime});
        if (ctxSwitch > 0) { gantt.push_back({-1, currentTime+ctxSwitch}); currentTime += ctxSwitch; }
    }

    displayGantt(gantt);
    displayTimeline(procs);
    displayResults(procs, "SJF — Non-Preemptive");
    checkStarvation(procs);
}

// ══════════════════════════════════════════
//  3. SJF Preemptive (SRTF)
// ══════════════════════════════════════════
void sjfPreemptive(vector<Process> procs) {
    int n = procs.size();
    vector<tuple<int,int,int>> gantt; // {pid, start, end}
    int currentTime = 0, completed = 0;
    int lastPid = -2, sliceStart = 0;

    while (completed < n) {
        int idx = -1, minR = INT_MAX;
        for (int i = 0; i < n; i++)
            if (procs[i].arrivalTime <= currentTime && procs[i].remainingTime > 0 &&
                procs[i].remainingTime < minR) { minR = procs[i].remainingTime; idx = i; }

        if (idx == -1) {
            if (lastPid != -1) { gantt.push_back({-1, currentTime, currentTime+1}); }
            else { get<2>(gantt.back()) = currentTime + 1; }
            lastPid = -1; currentTime++; continue;
        }

        if (procs[idx].firstRun) {
            procs[idx].responseTime = currentTime - procs[idx].arrivalTime;
            procs[idx].firstRun = false;
        }

        if (lastPid != procs[idx].id) {
            if (!gantt.empty()) get<2>(gantt.back()) = currentTime;
            gantt.push_back({procs[idx].id, currentTime, currentTime+1});
            if (!procs[idx].timeline.empty() && procs[idx].timeline.back().second == currentTime)
                procs[idx].timeline.back().second = currentTime + 1;
            else
                procs[idx].timeline.push_back({currentTime, currentTime+1});
        } else {
            get<2>(gantt.back()) = currentTime + 1;
            procs[idx].timeline.back().second = currentTime + 1;
        }

        lastPid = procs[idx].id;
        procs[idx].remainingTime--;
        currentTime++;

        if (procs[idx].remainingTime == 0) {
            procs[idx].completionTime = currentTime;
            procs[idx].turnaroundTime = procs[idx].completionTime - procs[idx].arrivalTime;
            procs[idx].waitingTime    = procs[idx].turnaroundTime - procs[idx].burstTime;
            completed++;
        }
    }

    // Merge consecutive gantt entries for same PID for cleaner display
    vector<tuple<int,int,int>> merged;
    for (auto& g : gantt) {
        if (!merged.empty() && get<0>(merged.back()) == get<0>(g))
            get<2>(merged.back()) = get<2>(g);
        else
            merged.push_back(g);
    }

    displayGanttFull(merged);
    displayTimeline(procs);
    displayResults(procs, "SJF Preemptive / SRTF");
    checkStarvation(procs);
}

// ══════════════════════════════════════════
//  4. Round Robin
// ══════════════════════════════════════════
void roundRobin(vector<Process> procs, int quantum) {
    int n = procs.size();
    sort(procs.begin(), procs.end(), [](const Process& a, const Process& b){
        return a.arrivalTime < b.arrivalTime;
    });

    queue<int> rq;
    vector<bool> inQueue(n, false);
    vector<pair<int,int>> gantt;
    int currentTime = 0, completed = 0;
    int ctxSwitch = globalSettings.contextSwitchEnabled ? globalSettings.contextSwitchTime : 0;

    rq.push(0); inQueue[0] = true;

    while (completed < n) {
        if (rq.empty()) {
            int next = INT_MAX;
            for (int i=0;i<n;i++) if (!inQueue[i] && procs[i].remainingTime > 0) next = min(next, procs[i].arrivalTime);
            gantt.push_back({-1, next}); currentTime = next;
            for (int i=0;i<n;i++) if (!inQueue[i] && procs[i].arrivalTime <= currentTime && procs[i].remainingTime > 0) { rq.push(i); inQueue[i] = true; }
            continue;
        }

        int idx = rq.front(); rq.pop();

        if (procs[idx].firstRun) {
            procs[idx].responseTime = currentTime - procs[idx].arrivalTime;
            procs[idx].firstRun = false;
        }

        int exec = min(quantum, procs[idx].remainingTime);
        procs[idx].timeline.push_back({currentTime, currentTime + exec});
        procs[idx].remainingTime -= exec;
        currentTime += exec;
        gantt.push_back({procs[idx].id, currentTime});

        for (int i=0;i<n;i++)
            if (!inQueue[i] && procs[i].arrivalTime <= currentTime && procs[i].remainingTime > 0) { rq.push(i); inQueue[i] = true; }

        if (procs[idx].remainingTime == 0) {
            procs[idx].completionTime = currentTime;
            procs[idx].turnaroundTime = procs[idx].completionTime - procs[idx].arrivalTime;
            procs[idx].waitingTime    = procs[idx].turnaroundTime - procs[idx].burstTime;
            completed++;
        } else {
            rq.push(idx);
        }

        if (ctxSwitch > 0 && !rq.empty()) { gantt.push_back({-1, currentTime+ctxSwitch}); currentTime += ctxSwitch; }
    }

    displayGantt(gantt);
    displayTimeline(procs);
    displayResults(procs, "Round Robin (Quantum=" + to_string(quantum) + ")");
    checkStarvation(procs);
}

// ══════════════════════════════════════════
//  5. Priority Scheduling with Aging
// ══════════════════════════════════════════
void priorityScheduling(vector<Process> procs, bool preemptive = false) {
    int n = procs.size();
    vector<bool> done(n, false);
    vector<pair<int,int>> gantt;
    int currentTime = 0, completed = 0;
    int ctxSwitch = globalSettings.contextSwitchEnabled ? globalSettings.contextSwitchTime : 0;
    // effective priority (modified by aging)
    vector<int> effPriority(n);
    for (int i=0;i<n;i++) effPriority[i] = procs[i].priority;

    if (!preemptive) {
        while (completed < n) {
            int idx = -1, best = INT_MAX;
            for (int i=0;i<n;i++)
                if (!done[i] && procs[i].arrivalTime <= currentTime && effPriority[i] < best) {
                    best = effPriority[i]; idx = i;
                }
            if (idx == -1) {
                int next = INT_MAX;
                for (int i=0;i<n;i++) if (!done[i]) next = min(next, procs[i].arrivalTime);
                gantt.push_back({-1, next}); currentTime = next; continue;
            }
            procs[idx].responseTime = currentTime - procs[idx].arrivalTime;
            procs[idx].timeline.push_back({currentTime, currentTime + procs[idx].burstTime});
            currentTime += procs[idx].burstTime;
            procs[idx].completionTime = currentTime;
            procs[idx].turnaroundTime = procs[idx].completionTime - procs[idx].arrivalTime;
            procs[idx].waitingTime    = procs[idx].turnaroundTime - procs[idx].burstTime;
            done[idx] = true; completed++;
            gantt.push_back({procs[idx].id, currentTime});
            if (ctxSwitch > 0) { gantt.push_back({-1, currentTime+ctxSwitch}); currentTime += ctxSwitch; }

            // Aging: boost priority of waiting processes
            if (globalSettings.agingEnabled) {
                for (int i=0;i<n;i++) {
                    if (!done[i] && procs[i].arrivalTime <= currentTime) {
                        procs[i].ageCounter += procs[idx].burstTime;
                        if (procs[i].ageCounter >= globalSettings.agingThreshold && effPriority[i] > 1) {
                            effPriority[i]--;
                            procs[i].ageCounter = 0;
                            cout << YELLOW << "  ⬆ Aging: " << procs[i].name
                                 << " priority boosted to " << effPriority[i] << "\n" << RESET;
                        }
                    }
                }
            }
        }
    }

    displayGantt(gantt);
    displayTimeline(procs);
    string title = preemptive ? "Priority Scheduling (Preemptive)" : "Priority Scheduling (Non-Preemptive)";
    if (globalSettings.agingEnabled) title += " + Aging";
    displayResults(procs, title);
    checkStarvation(procs);
}

// ══════════════════════════════════════════
//  6. Multilevel Queue Scheduling
// ══════════════════════════════════════════
void multilevelQueue(vector<Process> procs, int quantum) {
    // Queue 0: Round Robin (highest priority)
    // Queue 1: SJF
    // Queue 2: FCFS (lowest priority)

    printSectionHeader("Multilevel Queue Info");
    cout << "  " << PROC_COLORS[0] << WHITE << " Queue 0 " << RESET << "  Round Robin (Quantum=" << quantum << ")  — Highest Priority\n";
    cout << "  " << PROC_COLORS[1] << WHITE << " Queue 1 " << RESET << "  Shortest Job First (Non-Preemptive)\n";
    cout << "  " << PROC_COLORS[2] << WHITE << " Queue 2 " << RESET << "  First Come First Serve            — Lowest Priority\n\n";

    int n = procs.size();
    vector<pair<int,int>> gantt;
    vector<bool> done(n, false);
    int currentTime = 0, completed = 0;

    auto getAvailable = [&](int qLevel) -> vector<int> {
        vector<int> avail;
        for (int i=0;i<n;i++)
            if (!done[i] && procs[i].queueLevel == qLevel && procs[i].arrivalTime <= currentTime)
                avail.push_back(i);
        return avail;
    };

    auto anyPending = [&]() -> bool {
        for (int i=0;i<n;i++) if (!done[i]) return true;
        return false;
    };

    auto nextArrival = [&]() -> int {
        int t = INT_MAX;
        for (int i=0;i<n;i++) if (!done[i]) t = min(t, procs[i].arrivalTime);
        return t;
    };

    while (completed < n) {
        bool didWork = false;

        // ── Queue 0: RR ──
        auto q0 = getAvailable(0);
        if (!q0.empty()) {
            // pick first arrived
            sort(q0.begin(), q0.end(), [&](int a, int b){ return procs[a].arrivalTime < procs[b].arrivalTime; });
            int idx = q0[0];
            if (procs[idx].firstRun) { procs[idx].responseTime = currentTime - procs[idx].arrivalTime; procs[idx].firstRun = false; }
            int exec = min(quantum, procs[idx].remainingTime);
            procs[idx].timeline.push_back({currentTime, currentTime+exec});
            procs[idx].remainingTime -= exec;
            currentTime += exec;
            gantt.push_back({procs[idx].id, currentTime});
            if (procs[idx].remainingTime == 0) {
                procs[idx].completionTime = currentTime;
                procs[idx].turnaroundTime = procs[idx].completionTime - procs[idx].arrivalTime;
                procs[idx].waitingTime = procs[idx].turnaroundTime - procs[idx].burstTime;
                done[idx] = true; completed++;
            }
            didWork = true; continue;
        }

        // ── Queue 1: SJF ──
        auto q1 = getAvailable(1);
        if (!q1.empty()) {
            int idx = *min_element(q1.begin(), q1.end(), [&](int a, int b){ return procs[a].burstTime < procs[b].burstTime; });
            if (procs[idx].firstRun) { procs[idx].responseTime = currentTime - procs[idx].arrivalTime; procs[idx].firstRun = false; }
            procs[idx].timeline.push_back({currentTime, currentTime+procs[idx].burstTime});
            currentTime += procs[idx].burstTime;
            procs[idx].completionTime = currentTime;
            procs[idx].turnaroundTime = procs[idx].completionTime - procs[idx].arrivalTime;
            procs[idx].waitingTime = procs[idx].turnaroundTime - procs[idx].burstTime;
            done[idx] = true; completed++;
            gantt.push_back({procs[idx].id, currentTime});
            didWork = true; continue;
        }

        // ── Queue 2: FCFS ──
        auto q2 = getAvailable(2);
        if (!q2.empty()) {
            sort(q2.begin(), q2.end(), [&](int a, int b){ return procs[a].arrivalTime < procs[b].arrivalTime; });
            int idx = q2[0];
            if (procs[idx].firstRun) { procs[idx].responseTime = currentTime - procs[idx].arrivalTime; procs[idx].firstRun = false; }
            procs[idx].timeline.push_back({currentTime, currentTime+procs[idx].burstTime});
            currentTime += procs[idx].burstTime;
            procs[idx].completionTime = currentTime;
            procs[idx].turnaroundTime = procs[idx].completionTime - procs[idx].arrivalTime;
            procs[idx].waitingTime = procs[idx].turnaroundTime - procs[idx].burstTime;
            done[idx] = true; completed++;
            gantt.push_back({procs[idx].id, currentTime});
            didWork = true; continue;
        }

        if (!didWork && anyPending()) {
            gantt.push_back({-1, nextArrival()});
            currentTime = nextArrival();
        }
    }

    displayGantt(gantt);
    displayTimeline(procs);
    displayResults(procs, "Multilevel Queue (RR|SJF|FCFS)");
    checkStarvation(procs);
}

// ══════════════════════════════════════════
//  COMPARISON TABLE
// ══════════════════════════════════════════
void showComparisonTable() {
    printSectionHeader("Algorithm Comparison & Guide");

    cout << BOLD << "  " << left
         << setw(28) << "Algorithm"
         << setw(13) << "Preemptive"
         << setw(10) << "Overhead"
         << setw(12) << "Starvation"
         << "Best Use Case\n" << RESET;
    printDivider(false, 75);

    auto row = [](string algo, string pre, string oh, string starv, string use, string color) {
        cout << "  " << color << setw(28) << algo << RESET
             << setw(13) << pre << setw(10) << oh << setw(12) << starv << use << "\n";
    };

    row("FCFS",                   "No",   "Low",    "No",       "Batch, simple systems",         CYAN);
    row("SJF Non-Preemptive",     "No",   "Low",    "Yes",      "Minimize avg WT",               GREEN);
    row("SJF Preemptive (SRTF)",  "Yes",  "High",   "Yes",      "Optimal avg WT",                GREEN);
    row("Round Robin",            "Yes",  "Medium", "No",       "Time-sharing, fairness",        YELLOW);
    row("Priority (Non-Pre)",     "No",   "Low",    "Yes",      "Real-time, critical tasks",     MAGENTA);
    row("Multilevel Queue",       "Mix",  "Medium", "Possible", "Mixed workload systems",        BLUE);
    printDivider(true, 75);

    cout << "\n  " << BOLD << "Metrics Guide:\n\n" << RESET;
    cout << "  " << GREEN  << "TAT (Turnaround Time) " << RESET << "= Completion − Arrival\n";
    cout << "  " << YELLOW << "WT  (Waiting Time)    " << RESET << "= TAT − Burst\n";
    cout << "  " << CYAN   << "RT  (Response Time)   " << RESET << "= First CPU access − Arrival\n";
    cout << "  " << MAGENTA<< "CPU Utilization       " << RESET << "= Total Burst / Total Time × 100\n";
    printDivider();
}

// ══════════════════════════════════════════
//  SETTINGS MENU
// ══════════════════════════════════════════
void showSettings() {
    printSectionHeader("Settings");

    cout << "  Current Settings:\n\n";
    cout << "  [1] Context Switch Overhead  : "
         << (globalSettings.contextSwitchEnabled ? GREEN + string("ON") + RESET : RED + string("OFF") + RESET)
         << " (time = " << globalSettings.contextSwitchTime << ")\n";
    cout << "  [2] Process Aging (Priority) : "
         << (globalSettings.agingEnabled ? GREEN + string("ON") + RESET : RED + string("OFF") + RESET)
         << " (threshold = " << globalSettings.agingThreshold << ")\n";
    cout << "  [3] Save Report to File      : "
         << (globalSettings.saveReport ? GREEN + string("ON") + RESET : RED + string("OFF") + RESET)
         << " (" << globalSettings.reportFile << ")\n";
    cout << "  [0] Back\n\n  Choice: ";

    int c; cin >> c;
    switch(c) {
        case 1:
            globalSettings.contextSwitchEnabled = !globalSettings.contextSwitchEnabled;
            if (globalSettings.contextSwitchEnabled) {
                cout << "  Context switch time (units): "; cin >> globalSettings.contextSwitchTime;
            }
            break;
        case 2:
            globalSettings.agingEnabled = !globalSettings.agingEnabled;
            if (globalSettings.agingEnabled) {
                cout << "  Aging threshold (units): "; cin >> globalSettings.agingThreshold;
            }
            break;
        case 3:
            globalSettings.saveReport = !globalSettings.saveReport;
            if (globalSettings.saveReport) {
                if (globalSettings.saveReport) {
                    ofstream f(globalSettings.reportFile); // reset file
                    f << "CPU Scheduling Simulator — Report\n";
                    f << string(40,'=') << "\n";
                    f.close();
                }
            }
            break;
    }
}

// ══════════════════════════════════════════
//  MAIN
// ══════════════════════════════════════════
int main() {
    clearScreen();
    printHeader();

    int choice;
    do {
        cout << BOLD << BLUE << "\n  ╔═════════════════════════╗\n";
        cout << "  ║      MAIN MENU          ║\n";
        cout << "  ╠═════════════════════════╣\n" << RESET;
        cout << CYAN << "  ║ " << RESET << " 1. FCFS                " << CYAN << "║\n" << RESET;
        cout << CYAN << "  ║ " << RESET << " 2. SJF Non-Preemptive  " << CYAN << "║\n" << RESET;
        cout << CYAN << "  ║ " << RESET << " 3. SJF Preemptive(SRTF)" << CYAN << "║\n" << RESET;
        cout << CYAN << "  ║ " << RESET << " 4. Round Robin         " << CYAN << "║\n" << RESET;
        cout << CYAN << "  ║ " << RESET << " 5. Priority Scheduling " << CYAN << "║\n" << RESET;
        cout << CYAN << "  ║ " << RESET << " 6. Multilevel Queue    " << CYAN << "║\n" << RESET;
        cout << BLUE << "  ╠═════════════════════════╣\n" << RESET;
        cout << YELLOW << "  ║ " << RESET << " 7. Comparison Guide    " << YELLOW << "║\n" << RESET;
        cout << MAGENTA << "  ║ " << RESET << " 8. Settings            " << MAGENTA << "║\n" << RESET;
        cout << RED << "  ║ " << RESET << " 0. Exit                " << RED << "║\n" << RESET;
        cout << BLUE << "  ╚═════════════════════════╝\n" << RESET;
        cout << "\n  Enter choice: ";
        cin >> choice;

        switch (choice) {
            case 1: { auto p = getProcesses(false); fcfs(p); break; }
            case 2: { auto p = getProcesses(false); sjfNonPreemptive(p); break; }
            case 3: { auto p = getProcesses(false); sjfPreemptive(p); break; }
            case 4: {
                auto p = getProcesses(false);
                int q; cout << "\n  Time Quantum: "; cin >> q;
                roundRobin(p, q); break;
            }
            case 5: {
                auto p = getProcesses(true);
                priorityScheduling(p); break;
            }
            case 6: {
                auto p = getProcesses(true, true);
                int q; cout << "\n  RR Quantum for Queue 0: "; cin >> q;
                multilevelQueue(p, q); break;
            }
            case 7: showComparisonTable(); break;
            case 8: showSettings(); break;
            case 0:
                cout << CYAN << "\n  Thank you for using CPU Sim v2.0. Goodbye!\n\n" << RESET;
                break;
            default:
                cout << RED << "\n  ✘ Invalid choice.\n" << RESET;
        }

        if (choice != 0) {
            cout << DIM << "\n  Press Enter to return to menu...";
            cin.ignore(); cin.get();
            clearScreen();
            printHeader();
        }

    } while (choice != 0);

    return 0;
}
