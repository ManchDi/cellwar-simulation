#include <stdio.h>
#include <pthread.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>

using namespace std;

#define MAX_SOLIDERS 15

struct threadArgs {
int t1Players;
int t2Players;
};
struct soliderArgs {
int id;
int x;
int y;
};
struct Cells {
unsigned int own : 8;
};

std::unordered_map<int, std::unordered_map<int, int>> map;
std::pair<int, int> getRandom();
void majority(int x, int y, int team);
int shoot(int team,int id, int selfX, int selfY);
void drawMap1(int mapRows, int mapCols);
void editCell(int i, int j, int mapCols, Cells cell);
Cells getCell(int i, int j, int mapCols);
void writeMap(Cells cells[], int mapRows, int mapCols);
int getRandomInt();
void logMessage(const std::string& message);
std::string getTimeStamp();

pthread_mutex_t myMutex;
int shots = 0;
int winTarget;
int onesOnField = 0;
int twosOnField = 0;
int shotCounter = 0;
int mapRows = 0;
int mapCols = 0;
int zeros = 0;
atomic<long> a(0);

// Cross-platform clear screen
void clearScreen() {
#if defined(_WIN32)
system("cls");
#else
system("clear");
#endif
}

void * T1(void * arg){
soliderArgs* tArgs = static_cast<soliderArgs*>(arg);
int ThreadID = tArgs->id;
int selfX = tArgs->x;
int selfY = tArgs->y;
int team = 1;
int Numshots = 0;
while(true){
pthread_mutex_lock(&myMutex);
shoot(team, ThreadID, selfX, selfY);
Numshots++;
if(Numshots > 1){
Numshots = 0;
pthread_mutex_unlock(&myMutex);
std::this_thread::sleep_for(std::chrono::milliseconds(getRandomInt()));
} else pthread_mutex_unlock(&myMutex);
}
delete tArgs;
return nullptr;
}

void * T2(void * arg){
soliderArgs* tArgs = static_cast<soliderArgs*>(arg);
int ThreadID = tArgs->id;
int selfX = tArgs->x;
int selfY = tArgs->y;
int team = 2;
int Numshots = 0;
printf("Solider[2]-[%d] is ready!\n", ThreadID);

while(true){
    pthread_mutex_lock(&myMutex);        
    shoot(team, ThreadID, selfX, selfY);
    Numshots++;
    if(Numshots > 1){
        Numshots = 0;
        pthread_mutex_unlock(&myMutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(getRandomInt()));
    } else pthread_mutex_unlock(&myMutex);
}    
delete tArgs;
return nullptr;

}

void * controllerThread(void* arg){
threadArgs* tArgs = static_cast<threadArgs*>(arg);
int t1Players = tArgs->t1Players;
int t2Players = tArgs->t2Players;

pthread_t T1Threads[t1Players];
pthread_t T2Threads[t2Players];
pthread_mutex_lock(&myMutex);
int p = 6;
drawMap1(mapRows, mapCols);

for(int i = 0; i < (t1Players + t2Players); i++){
    int x1, y1;
    while (true){
        if(i >= t1Players) p = 9;
        std::pair<int, int> coords = getRandom();
        x1 = coords.first;
        y1 = coords.second;
        Cells cell = getCell(y1, x1, mapCols);
        if(cell.own != 0) continue;
        else {cell.own = p; editCell(y1, x1, mapCols, cell); zeros--; break;}
    } 
    soliderArgs* args = new soliderArgs{i >= t1Players ? (i - t1Players) : i, x1, y1};
    if (p == 6) pthread_create(&T1Threads[i], nullptr, T1, args);
    else pthread_create(&T2Threads[i - t1Players], nullptr, T2, args);
}
pthread_mutex_unlock(&myMutex);

while(true){        
    if (zeros < 1){
        pthread_mutex_lock(&myMutex);
        try {
            for(int i = 0; i < t1Players; i++) pthread_cancel(T1Threads[i]);
            for(int i = 0; i < t2Players; i++) pthread_cancel(T2Threads[i]);
            pthread_mutex_unlock(&myMutex);
            for(int i = 0; i < t1Players; i++) pthread_join(T1Threads[i], nullptr);
            for(int i = 0; i < t2Players; i++) pthread_join(T2Threads[i], nullptr);
        } catch (std::exception const& ex){pthread_mutex_unlock(&myMutex); std::cerr << "Exception: " << ex.what() << std::endl;}
        printf("CANCELLED ALL THREADS\n");
        return nullptr;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
return nullptr;

}

// Optional: small tweak for shoot to call drawMap1 less frequently
int shoot(int team ,int ThreadID, int xSelf, int ySelf){
    if(zeros<1) return 0;
    std::pair<int,int> coords = getRandom();
    int x = coords.first;
    int y = coords.second;
    bool selfAttack = false;

    Cells cell = getCell(y,x,mapCols);

    if(cell.own==0){
        cell.own = team;
        if(team==2) twosOnField++; else onesOnField++;
        zeros--;
    } else if(cell.own==team){
        cell.own=0;
        if(team==1) onesOnField--; else twosOnField--;
        zeros++;
        selfAttack=true;
    } else if(cell.own==6||cell.own==9){
        if((team==1 && cell.own==6) || (team==2 && cell.own==9)) selfAttack=true;
    } else {
        cell.own=team;
        if(team==2){ onesOnField--; twosOnField++; } 
        else { twosOnField--; onesOnField++; }
    }

    editCell(y,x,mapCols,cell);
    if(!selfAttack) majority(x,y,team);

    shotCounter++;
    if(shotCounter>=2){  
        system("clear"); // clear terminal
        drawMap1(mapRows,mapCols);
        shotCounter=0;
    }

    // logging as before
    std::stringstream ss;
    ss << "Solider from team " << team <<"["<<ThreadID<<"] shot at ("<<x<<","<<y<<")"
       << " from ("<<xSelf<<","<<ySelf<<")"<<" date:"<<getTimeStamp()<<"\n";
    logMessage(ss.str());

    return 0;
}

void majority(int x, int y, int team){
bool readMode = true;
int ones = 0;
int twos = 0;

for(int dx = -1; dx <= 1; dx++){
    for(int dy = -1; dy <= 1; dy++){
        int nx = x + dx;
        int ny = y + dy;
        if(nx >= 0 && nx < mapCols && ny >= 0 && ny < mapRows){
            Cells cell = getCell(ny, nx, mapCols);
            unsigned int val = cell.own;
            if(!readMode){
                if(ones > twos && val != 6 && val != 9 && team == 1){ if(val == 2) twosOnField--; if(val == 0) zeros--; val = 1; if(!(x==nx && y==ny)) onesOnField++; }
                else if(ones < twos && val != 6 && val != 9 && team == 2){ if(val == 1) onesOnField--; if(val == 0) zeros--; val = 2; if(!(x==nx && y==ny)) twosOnField++; }
                cell.own = val;
                editCell(ny, nx, mapCols, cell);
            } else {
                if(val==1||val==6) ones++;
                else if(val==2||val==9) twos++;
            }
        }
    }
    if(dx == 1 && readMode){ dx = -2; readMode = false; }
}

}

std::string getTimeStamp(){
auto now = std::chrono::system_clock::now();
auto now_c = std::chrono::system_clock::to_time_t(now);
std::stringstream ss;
ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
return ss.str();
}

void logMessage(const std::string& message){
std::ofstream logFile("log.txt", std::ios::app);
if(logFile.is_open()){ logFile << message << std::endl; logFile.close(); }
else std::cerr << "Failed to open log file!" << std::endl;
}

std::pair<int,int> getRandom(){
std::chrono::high_resolution_clock::time_point tp = std::chrono::high_resolution_clock::now();
std::mt19937 gen(tp.time_since_epoch().count());
std::uniform_int_distribution<int> dist(0, mapCols-1);
int x = dist(gen);
std::mt19937 gen1(tp.time_since_epoch().count()+2);
std::uniform_int_distribution<int> dist1(0, mapRows-1);
int y = dist1(gen1);
return {x,y};
}

int getRandomInt(){
std::chrono::high_resolution_clock::time_point tp = std::chrono::high_resolution_clock::now();
std::mt19937 gen(tp.time_since_epoch().count());
std::uniform_int_distribution<int> dist(1,3);
return dist(gen)*100;
}

int getMap(Cells* cells, int mapRows, int mapCols){
ifstream infile("map.bin", ios::in | ios::binary);
if(!infile.is_open()){ cerr << "Failed to open file!" << endl; return 1; }
infile.read(reinterpret_cast<char*>(cells), mapRows*mapCols*sizeof(Cells));
infile.close();
return 0;
}

// Updated drawMap1: color-coded map display
void drawMap1(int mapRows, int mapCols){
    Cells tempCells[mapRows*mapCols];
    getMap(tempCells, mapRows, mapCols);

    std::cout << "Team 1: 🔴  Team 2: 🔵  Empty: .\n\n";

    // Print column indices
    std::cout << "   ";
    for(int j = 0; j < mapCols; j++)
        std::cout << j%10 << " "; // keep single digit for readability
    std::cout << "\n";

    for(int i = 0; i < mapRows; i++){
        // Print row index
        std::cout << i%10 << " |";

        for(int j = 0; j < mapCols; j++){
            int index = i*mapCols + j;
switch(tempCells[index].own){
    case 0: std::cout << "\033[0;37m.\033[0m "; break;      // empty
    case 1: std::cout << "\033[1;31m🔴\033[0m "; break;      // team 1 (majority)
    case 2: std::cout << "\033[1;34m🔵\033[0m "; break;      // team 2 (majority)
    case 6: std::cout << "\033[1;31m🔴\033[0m "; break;      // team 1 soldier
    case 9: std::cout << "\033[1;34m🔵\033[0m "; break;      // team 2 soldier
    default: std::cout << "\033[0;33m?\033[0m "; break;     // fallback in yellow
}
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}



Cells getCell(int i, int j, int mapCols){
fstream mapFile("map.bin", ios::in | ios::binary);
Cells cell{};
if(!mapFile.is_open()) cerr << "Failed to open map file to read." << endl;
mapFile.seekg((i*mapCols+j)*sizeof(Cells));
mapFile.read(reinterpret_cast<char*>(&cell), sizeof(Cells));
mapFile.close();
return cell;
}

void editCell(int i, int j, int mapCols, Cells cell){
fstream outfile("map.bin", ios::in | ios::out | ios::binary);
if(!outfile.is_open()){ cerr << "Failed to open file for writing!" << endl; return; }
outfile.seekp((i*mapCols+j)*sizeof(Cells));
outfile.write(reinterpret_cast<const char*>(&cell), sizeof(Cells));
outfile.close();
}

void writeMap(Cells* cells, int mapRows, int mapCols){
ofstream outfile("map.bin", ios::out | ios::binary);
if(!outfile.is_open()){ cerr << "Failed to open file!" << endl; return; }
outfile.write(reinterpret_cast<const char*>(cells), mapRows*mapCols*sizeof(Cells));
outfile.close();
cout << "Wrote original map to file\n";
}

int main(int argc, char *argv[]){
std::string info="new simulation";
std::string end="simulation is over";
logMessage(info);

if(argc != 5){ cout << "must have exactly 4 arguments" << endl; return 1; }

mapRows = stoi(argv[3]);
mapCols = stoi(argv[4]);
threadArgs threadArgs;
int totalCells = mapRows * mapCols;
Cells cells[totalCells];
int p1 = stoi(argv[1]);
int p2 = stoi(argv[2]);
threadArgs.t1Players = p1;
threadArgs.t2Players = p2;

winTarget = totalCells - (p1 + p2);

if(mapRows<1||mapRows>50 || mapCols<1||mapCols>50 || p1<1||p2<1 || p1+p2>=totalCells){
    cout << "Invalid parameters" << endl;
    return 1;
}

for(int i=0;i<totalCells;i++){ cells[i].own=0; zeros++; }
writeMap(cells,mapRows,mapCols);

pthread_t ctrl;
pthread_create(&ctrl,nullptr,controllerThread,&threadArgs);
pthread_join(ctrl,nullptr);

logMessage(end);

std::ifstream logFile("log.txt");
std::string s;
if(logFile.is_open()){
    while(std::getline(logFile,s)) cout << s << endl;
    logFile.close();
}

drawMap1(mapRows,mapCols);
printf("\nTotal \n-shots:%d\n-onesOnField:%d\n-twosOnField:%d\n-zeros:%d\n", shots, onesOnField, twosOnField, zeros);

return 0;

}
