#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>

using namespace std;
typedef unsigned long long ull;
typedef pair<int, int> LINE;
typedef vector<LINE> VLINE;
typedef pair<VLINE, int> SET;
const int K = 1024;

// 默认参数
/*  cache_size: 8K, 16K, 32K, 64K;
    cache_asso: 1, 2, 4, 8;
    block_size: 16, 32, 64, 128 */
int cache_size = 64 * K; // 默认缓存大小
int cache_asso = 4; // 默认缓存关联度
int block_size = 16; // 默认块大小

int block_num = 0; // 块数
int set_num = 0; // 组数

ull read_cnt = 0;
ull write_cnt = 0;
ull inst_cnt = 0;
ull read_miss_cnt = 0;
ull write_miss_cnt = 0;
ull inst_miss_cnt = 0;

const int MAX_BLOCK_NUM = 64 * K / 16; // 最大块数 4096
const int MAX_SET_NUM = MAX_BLOCK_NUM / 1; // 最大组数 4096
const int MAX_LINE_NUM = 8; // 每组最大行数 8

// vector<pair<vector<pair<int, int>>, int>>
LINE tmp = make_pair(-1, 0);
vector<SET> cache(MAX_SET_NUM, SET(VLINE(MAX_LINE_NUM, tmp), 0));

vector<string> split(const string &s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool handler(uint32_t addr){
    int no_block = addr / block_size;
    int no_set = no_block % set_num;
    int tag = no_block / set_num;

    for(int i = 0; i < cache_asso; i++){
        if(cache[no_set].first[i].first == tag){
            int time = cache[no_set].first[i].second;
            for(int j = 0; j < cache_asso; j++){
                if(cache[no_set].first[j].second > time){
                    cache[no_set].first[j].second--;
                }
            }
            cache[no_set].first[i].second = cache[no_set].second;
            return true;
        }
    }
    // miss
    for(int i = 0; i < cache_asso; i++){
        if(cache[no_set].first[i].first == -1){
            // empty
            cache[no_set].first[i].first = tag;
            cache[no_set].first[i].second = ++(cache[no_set].second);
            return false;
        }
    }
    // full
    int min_index = 0;
    for(int i = 0; i < cache_asso; i++){
        if(cache[no_set].first[i].second == 1){
            min_index = i;
        }
        else{
            cache[no_set].first[i].second--;
        }
    }
    cache[no_set].first[min_index].first = tag;
    cache[no_set].first[min_index].second = cache[no_set].second;
    return false;
}

bool read_handler(uint32_t addr){
    read_cnt++;
    bool ret = handler(addr);
    if(!ret)
        read_miss_cnt++;
    return ret;
}

bool write_handler(uint32_t addr){
    write_cnt++;
    bool ret = handler(addr);
    if(!ret)
        write_miss_cnt++;
    return ret;
}

bool inst_handler(uint32_t addr){
    inst_cnt++;
    bool ret = handler(addr);
    if(!ret)
        inst_miss_cnt++;
    return ret;
}

void print(string op, uint32_t addr = 0, bool hit = false){
    printf("Total Access: %-8llu, Totol Miss Count: %-8llu, Totol Miss Rate: %3.2f%%\n",
            read_cnt + write_cnt + inst_cnt, read_miss_cnt + write_miss_cnt + inst_miss_cnt,
            (double)(read_miss_cnt + write_miss_cnt + inst_miss_cnt) / (read_cnt + write_cnt + inst_cnt) * 100.0);
    printf("Inst  Access: %-8llu, Inst  Miss Count: %-8llu, Inst  Miss Rate: %3.2f%%\n",
            inst_cnt, inst_miss_cnt, (double)inst_miss_cnt / inst_cnt * 100.0);
    printf("Read  Access: %-8llu, Read  Miss Count: %-8llu, Read  Miss Rate: %3.2f%%\n",
            read_cnt, read_miss_cnt, (double)read_miss_cnt / read_cnt * 100.0);
    printf("Write Access: %-8llu, Write Miss Count: %-8llu, Write Miss Rate: %3.2f%%\n",
            write_cnt, write_miss_cnt, (double)write_miss_cnt / write_cnt * 100.0);
    puts(" ");
    if(strcmp(op.c_str(), "end") == 0)
        return;
    if(strcmp(op.c_str(), "0") == 0)
        printf("Access Address: %-6s,       Address: 0x%x\n", "Read", addr);
    else if(strcmp(op.c_str(), "1") == 0)
        printf("Access Address: %-6s,       Address: 0x%x\n", "Write", addr);
    else
        printf("Access Address: %-6s,       Address: 0x%x\n", "Inst", addr);
    int no_block = addr / block_size;
    int no_set = no_block % set_num;
    int offset = addr % block_size;
    printf("No. of Blocks: %-10d, Addr in Block: %-8d, No. of Sets(Index): %-5d\n",
            no_block, offset, no_set);
    if(hit){
        printf("Cache Hit: %s\n", "Yes");
    }
    else{
        printf("Cache Hit: %s\n", "No");
    }
    puts(" ");
}

int main(int argc, char* argv[]) {
    string file_addr = "tracefiles/085.gcc.din"; // 默认文件地址
    // string file_addr = "trace files/test.din"; // 默认文件地址
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--file_addr" && i + 1 < argc) {
            file_addr = argv[++i];
        } else if (arg == "--cache_size" && i + 1 < argc) {
            cache_size = stoi(argv[++i]) * K;
        } else if (arg == "--block_size" && i + 1 < argc) {
            block_size = stoi(argv[++i]);
        } else if (arg == "--cache_asso" && i + 1 < argc) {
            cache_asso = stoi(argv[++i]);
        } else {
            cerr << "Unknown option: " << arg << endl;
        }
    }

    ifstream inputFile(file_addr);
    
    if (!inputFile) {
        cerr << "Unable to open file " << file_addr;
    }
    block_num = cache_size / block_size; // 块数
    set_num = block_num / cache_asso; // 组数
    
    string line;
    bool to_end = true;
    while (getline(inputFile, line)) {
        vector<string> tokens = split(line, ' '); 
        string op = tokens[0];
        uint32_t addr = stoul(tokens[1], 0, 16);
        bool hit = false;
        if(strcmp(op.c_str(), "0") == 0){ // Load Data (Read)
            hit = read_handler(addr);
        }
        else if(strcmp(op.c_str(), "1") == 0){ // Store Data (Write)
            hit = write_handler(addr);
        }
        else{
            hit = inst_handler(addr);
        }
        if(!to_end){
            print(op, addr, hit);
            string next;
            cout << "Press AnyKey(except E and Q) to continue or E to the end or Q to quit..." << endl;
            getline(cin, next);
            if(strcmp(next.c_str(), "E") == 0 || strcmp(next.c_str(), "e") == 0){
                to_end = true;
                continue;
            }
            else if(strcmp(next.c_str(), "Q") == 0 || strcmp(next.c_str(), "q") == 0)
            {
                break;
            }
        }
    }
    if(to_end){
        print("end");
    }
    
    inputFile.close();
    return 0;
}