#include <bits/stdc++.h>
using namespace std ;

// ==================== STRUCTS ====================

typedef struct {
    int  head ;             // current head position
    int  disk_size ;        // total number of cylinders
    int  n ;                // number of requests
    int  req[100] ;         // request queue
} DiskState ;

/*
 *  seek_log[] : stores each individual step distance for a given run.
 *  Used to compute Variance = Sum[(di - avg)^2] / n
 *  avg_seek   : Total / n  (average seek distance per request)
 *  variance   : measure of fairness — lower variance = more uniform service
 */
typedef struct {
    int    total_seek ;
    double avg_seek   ;
    double variance   ;
} AlgoMetric ;

typedef struct {
    AlgoMetric fcfs  ;
    AlgoMetric sstf  ;
    AlgoMetric scan  ;
    AlgoMetric cscan ;
} Result ;

// ==================== MATH HELPERS ====================

/*
 *  computeMetrics  — fills an AlgoMetric from a raw seek-log array.
 *
 *  avg  = total / n
 *  var  = [ Sum (d_i - avg)^2 ] / n
 *
 *  Variance is used as a 'Fairness Index':
 *  a low-variance algorithm services requests more uniformly (no starvation).
 */
AlgoMetric computeMetrics(int* log, int n) {
    AlgoMetric m ;
    m.total_seek = 0 ;
    for(int i = 0 ; i < n ; i++)   m.total_seek += log[i] ;

    m.avg_seek = (n > 0) ? (double)m.total_seek / n : 0.0 ;

    double sumSq = 0.0 ;
    for(int i = 0 ; i < n ; i++) {
        double diff = log[i] - m.avg_seek ;
        sumSq += diff * diff ;
    }
    m.variance = (n > 0) ? sumSq / n : 0.0 ;

    return m ;
}

// ==================== SEEK HELPERS ====================

int seekDist(int a, int b) {
    return abs(a - b) ;
}

bool isVisited(bool* vis, int idx) {
    return vis[idx] ;
}

int findNearest(int* req, bool* vis, int n, int head) {
    int idx = -1, minD = INT_MAX ;
    for(int j = 0 ; j < n ; j++) {
        if(!isVisited(vis, j) && seekDist(head, req[j]) < minD) {
            minD = seekDist(head, req[j]) ;
            idx  = j ;
        }
    }
    return idx ;
}

void partitionLR(int* req, int n, int head, vector<int>& lft, vector<int>& rgt) {
    lft.clear() ;
    rgt.clear() ;
    for(int i = 0 ; i < n ; i++) {
        if(req[i] < head)   lft.push_back(req[i]) ;
        else                rgt.push_back(req[i]) ;
    }
    sort(lft.begin(), lft.end()) ;
    sort(rgt.begin(), rgt.end()) ;
}

// ==================== PRINT HELPERS ====================

void printBanner() {
    printf("\n") ;
    printf("  +================================================================+\n") ;
    printf("  |       DISK HEAD MOVEMENT ANALYSIS  ~  OS Scheduling Lab        |\n") ;
    printf("  |          Algorithms: FCFS | SSTF | SCAN | C-SCAN               |\n") ;
    printf("  |              Metrics: Seek Time | Avg | Variance    B=\\         |\n") ;
    printf("  +================================================================+\n") ;
    printf("\n") ;
}

void printMenu() {
    printf("\n") ;
    printf("  +--------------------------------+\n") ;
    printf("  |        MAIN  MENU     :>       |\n") ;
    printf("  |  [1]  Run FCFS                 |\n") ;
    printf("  |  [2]  Run SSTF                 |\n") ;
    printf("  |  [3]  Run SCAN                 |\n") ;
    printf("  |  [4]  Run C-SCAN               |\n") ;
    printf("  |  [5]  Run ALL + Compare        |\n") ;
    printf("  |  [6]  Random Workload Test     |\n") ;
    printf("  |  [7]  Re-enter Input           |\n") ;
    printf("  |  [0]  Exit             ;]      |\n") ;
    printf("  +--------------------------------+\n") ;
    printf("  Choice:\t") ;
}

void printSeekPath(const char* label, int init_head) {
    printf("\n  [ %-6s ]  Seek Path:\n  %d", label, init_head) ;
}

void printStep(int pos) {
    printf(" -> %d", pos) ;
}

void printMetrics(const char* label, AlgoMetric m) {
    printf("\n") ;
    printf("  [ %s ]  Total Seek : %d\n",   label, m.total_seek) ;
    printf("  [ %s ]  Avg / Req  : %.2f\n", label, m.avg_seek)   ;
    printf("  [ %s ]  Variance   : %.2f\n", label, m.variance)   ;
}

void printDivider() {
    printf("  ................................................................\n") ;
}

/*
 *  printCompareTable  — 4-algorithm comparison with three metrics.
 *
 *  Columns:
 *    Algorithm  |  Total Seek  |  Avg Seek (per req)  |  Variance (fairness)
 *
 *  Variance Interpretation (printed as footnote):
 *    Low  variance  =>  uniform service, fewer starvation risks
 *    High variance  =>  some requests served much faster/slower than average
 */
void printCompareTable(Result r) {
    printf("\n") ;
    printf("  +----------+-------------+-------------+------------------+\n") ;
    printf("  |  ALGO    | Total Seek  |  Avg / Req  | Variance (Fair.) |\n") ;
    printf("  +----------+-------------+-------------+------------------+\n") ;
    printf("  |  FCFS    |  %-9d  |  %-9.2f  |  %-14.2f  |\n",
           r.fcfs.total_seek,  r.fcfs.avg_seek,  r.fcfs.variance)  ;
    printf("  |  SSTF    |  %-9d  |  %-9.2f  |  %-14.2f  |\n",
           r.sstf.total_seek,  r.sstf.avg_seek,  r.sstf.variance)  ;
    printf("  |  SCAN    |  %-9d  |  %-9.2f  |  %-14.2f  |\n",
           r.scan.total_seek,  r.scan.avg_seek,  r.scan.variance)  ;
    printf("  |  C-SCAN  |  %-9d  |  %-9.2f  |  %-14.2f  |\n",
           r.cscan.total_seek, r.cscan.avg_seek, r.cscan.variance) ;
    printf("  +----------+-------------+-------------+------------------+\n") ;
    printf("  NOTE: Variance measures fairness -- lower = more uniform service.\n") ;
}

/*
 *  printBest  — dual-criterion winner analysis.
 *
 *  Two winners are reported:
 *    1.  Minimum Total Seek  =>  Best throughput
 *    2.  Minimum Variance    =>  Best fairness (no starvation)
 *
 *  A combined score (normalized sum) picks the overall recommended algorithm.
 *  Score = (seek / max_seek) + (var / max_var)  ;  lower score = better.
 */
void printBest(Result r) {
    const char* names[4] = {"FCFS", "SSTF", "SCAN", "C-SCAN"} ;
    int    seeks[4]  = { r.fcfs.total_seek, r.sstf.total_seek,
                         r.scan.total_seek, r.cscan.total_seek } ;
    double vars[4]   = { r.fcfs.variance, r.sstf.variance,
                         r.scan.variance,  r.cscan.variance } ;

    // --- find min seek ---
    int bestSeekIdx = 0 ;
    for(int i = 1 ; i < 4 ; i++)
        if(seeks[i] < seeks[bestSeekIdx])   bestSeekIdx = i ;

    // --- find min variance ---
    int bestVarIdx = 0 ;
    for(int i = 1 ; i < 4 ; i++)
        if(vars[i] < vars[bestVarIdx])   bestVarIdx = i ;

    // --- combined normalized score ---
    int    maxSeek = *max_element(seeks, seeks + 4) ;
    double maxVar  = *max_element(vars,  vars  + 4) ;
    int    bestCombIdx = 0 ;
    double bestScore   = 1e18 ;

    for(int i = 0 ; i < 4 ; i++) {
        double normSeek = (maxSeek > 0) ? (double)seeks[i] / maxSeek : 0.0 ;
        double normVar  = (maxVar  > 0) ? vars[i] / maxVar            : 0.0 ;
        double score    = normSeek + normVar ;
        if(score < bestScore) {
            bestScore   = score ;
            bestCombIdx = i ;
        }
    }

    printf("\n") ;
    printf("  +----------------------------------------------------------+\n") ;
    printf("  |                  ANALYSIS SUMMARY  ^u^                   |\n") ;
    printf("  +----------------------------------------------------------+\n") ;
    printf("  |  Best Throughput (min seek)  =>  %-6s  (seek = %d)\n",
           names[bestSeekIdx], seeks[bestSeekIdx]) ;
    printf("  |  Best Fairness   (min var.)  =>  %-6s  (var  = %.2f)\n",
           names[bestVarIdx],  vars[bestVarIdx]) ;
    printf("  |  Overall Recommended         =>  %-6s  (combined score)\n",
           names[bestCombIdx]) ;
    printf("  +----------------------------------------------------------+\n\n") ;
}

// ==================== INPUT ====================

void inputDisk(DiskState* ds) {
    input_start:
    printf("\n  Enter number of disk requests (max 100):\t") ;
    cin >> ds->n ;
    if(ds->n <= 0 || ds->n > 100) {
        printf("  [!] Must be between 1 and 100. Try again.\n") ;
        goto input_start ;
    }

    printf("  Enter %d request(s) (cylinder numbers):\n", ds->n) ;
    for(int i = 0 ; i < ds->n ; i++) {
        printf("    req[%d]:\t", i+1) ;
        cin >> ds->req[i] ;
    }

    head_input:
    printf("  Enter initial head position:\t") ;
    cin >> ds->head ;
    if(ds->head < 0) {
        printf("  [!] Head position can't be negative.\n") ;
        goto head_input ;
    }

    disk_input:
    printf("  Enter total disk size (number of cylinders):\t") ;
    cin >> ds->disk_size ;
    if(ds->disk_size <= ds->head) {
        printf("  [!] Disk size must be greater than head position.\n") ;
        goto disk_input ;
    }

    printf("\n  Input recorded. Ready to schedule.  ;]\n") ;
}

// ==================== ALGORITHMS ====================

AlgoMetric runFCFS(DiskState* ds) {
    int seek_log[100] ;
    int head = ds->head ;
    printSeekPath("FCFS", head) ;

    for(int i = 0 ; i < ds->n ; i++) {
        seek_log[i] = seekDist(head, ds->req[i]) ;
        head        = ds->req[i] ;
        printStep(head) ;
    }

    AlgoMetric m = computeMetrics(seek_log, ds->n) ;
    printMetrics("FCFS", m) ;
    return m ;
}

AlgoMetric runSSTF(DiskState* ds) {
    int   seek_log[100] ;
    bool  vis[100] = {false} ;
    int   head = ds->head ;
    printSeekPath("SSTF", head) ;

    for(int i = 0 ; i < ds->n ; i++) {
        int idx      = findNearest(ds->req, vis, ds->n, head) ;
        vis[idx]     = true ;
        seek_log[i]  = seekDist(head, ds->req[idx]) ;
        head         = ds->req[idx] ;
        printStep(head) ;
    }

    AlgoMetric m = computeMetrics(seek_log, ds->n) ;
    printMetrics("SSTF", m) ;
    return m ;
}

AlgoMetric runSCAN(DiskState* ds) {
    int         seek_log[100] ;
    int         logIdx = 0 ;
    int         head   = ds->head ;
    vector<int> lft, rgt ;
    partitionLR(ds->req, ds->n, head, lft, rgt) ;

    printSeekPath("SCAN", head) ;

    for(int x : rgt) {
        seek_log[logIdx++] = seekDist(head, x) ;
        head = x ;
        printStep(head) ;
    }

    // SCAN goes to the physical disk boundary before reversing
    seek_log[logIdx++] = seekDist(head, ds->disk_size - 1) ;
    head = ds->disk_size - 1 ;
    printStep(head) ;

    for(int i = (int)lft.size() - 1 ; i >= 0 ; i--) {
        seek_log[logIdx++] = seekDist(head, lft[i]) ;
        head = lft[i] ;
        printStep(head) ;
    }

    AlgoMetric m = computeMetrics(seek_log, logIdx) ;
    printMetrics("SCAN", m) ;
    return m ;
}

AlgoMetric runCSCAN(DiskState* ds) {
    int         seek_log[100] ;
    int         logIdx = 0 ;
    int         head   = ds->head ;
    vector<int> lft, rgt ;
    partitionLR(ds->req, ds->n, head, lft, rgt) ;

    printSeekPath("C-SCAN", head) ;

    for(int x : rgt) {
        seek_log[logIdx++] = seekDist(head, x) ;
        head = x ;
        printStep(head) ;
    }

    // C-SCAN goes to end boundary, wraps to 0 (counts the jump)
    seek_log[logIdx++] = seekDist(head, ds->disk_size - 1) + (ds->disk_size - 1) ;
    printf(" -> %d -> 0", ds->disk_size - 1) ;
    head = 0 ;

    for(int x : lft) {
        seek_log[logIdx++] = seekDist(head, x) ;
        head = x ;
        printStep(head) ;
    }

    AlgoMetric m = computeMetrics(seek_log, logIdx) ;
    printMetrics("C-SCAN", m) ;
    return m ;
}

// ==================== RUN ALL ====================

Result runAll(DiskState* ds) {
    Result r ;
    printDivider() ;
    r.fcfs  = runFCFS(ds)  ;
    printDivider() ;
    r.sstf  = runSSTF(ds)  ;
    printDivider() ;
    r.scan  = runSCAN(ds)  ;
    printDivider() ;
    r.cscan = runCSCAN(ds) ;
    printDivider() ;
    return r ;
}

// ==================== STRESS TEST ====================

/*
 *  stressTest  —  Random Workload Generator.
 *
 *  Prompts for N and disk size, seeds rand() with current time,
 *  then fills the request queue with random cylinder numbers
 *  in the range [0, disk_size-1] and auto-runs all algorithms.
 *
 *  Useful for performance benchmarking without manual input.
 */
void stressTest(DiskState* ds) {
    printf("\n") ;
    printf("  +------------------------------------------+\n") ;
    printf("  |   RANDOM WORKLOAD GENERATOR  ~  STRESS   |\n") ;
    printf("  +------------------------------------------+\n") ;

    stress_n:
    printf("  Enter number of random requests (max 100):\t") ;
    cin >> ds->n ;
    if(ds->n <= 0 || ds->n > 100) {
        printf("  [!] Must be between 1 and 100.\n") ;
        goto stress_n ;
    }

    stress_disk:
    printf("  Enter disk size (cylinders):\t") ;
    cin >> ds->disk_size ;
    if(ds->disk_size <= 1) {
        printf("  [!] Disk size must be > 1.\n") ;
        goto stress_disk ;
    }

    srand((unsigned int)time(NULL)) ;
    ds->head = rand() % ds->disk_size ;

    printf("  Generating %d random requests on a %d-cylinder disk...\n",
           ds->n, ds->disk_size) ;

    printf("  Head starts at:  %d\n", ds->head) ;
    printf("  Request queue:   ") ;
    for(int i = 0 ; i < ds->n ; i++) {
        ds->req[i] = rand() % ds->disk_size ;
        printf("%d ", ds->req[i]) ;
    }
    printf("\n") ;

    printf("\n  Running all algorithms on random workload...\n") ;
    Result r = runAll(ds) ;
    printCompareTable(r) ;
    printBest(r) ;
}

// ==================== MENU + MAIN ====================

int main() {
    DiskState ds  ;
    Result    res ;
    int       ch  ;

    // zero-init result
    memset(&res, 0, sizeof(Result)) ;

    printBanner() ;
    inputDisk(&ds) ;

    do {
        printMenu() ;
        cin >> ch ;

        switch(ch) {
            case 1:
                printDivider() ;
                res.fcfs = runFCFS(&ds) ;
                printDivider() ;
                break ;

            case 2:
                printDivider() ;
                res.sstf = runSSTF(&ds) ;
                printDivider() ;
                break ;

            case 3:
                printDivider() ;
                res.scan = runSCAN(&ds) ;
                printDivider() ;
                break ;

            case 4:
                printDivider() ;
                res.cscan = runCSCAN(&ds) ;
                printDivider() ;
                break ;

            case 5:
                res = runAll(&ds) ;
                printCompareTable(res) ;
                printBest(res) ;
                break ;

            case 6:
                stressTest(&ds) ;
                memset(&res, 0, sizeof(Result)) ;
                break ;

            case 7:
                inputDisk(&ds) ;
                memset(&res, 0, sizeof(Result)) ;
                break ;

            case 0:
                printf("\n  Exiting... Have a great OS lab!  ^u^\n\n") ;
                break ;

            default:
                printf("  [!] Invalid choice. Try again.\n") ;
        }

    } while(ch != 0) ;

    return 0 ;
}