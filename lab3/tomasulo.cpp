#include<iostream>
#include<vector>
#include<string.h>
#include<stdio.h>
using namespace std;

enum state {unready = 4, issue = 0, execmpl = 1, writebk = 2};

struct RS{
    char * name;
    int time;
    bool busy;
    char * op;
    char * Qj, * Qk;
    char * Vj, * Vk;
    int inst_idex;
    RS(char * Sname, int Stime, bool Sbusy, char * Sop, char * SQj, char * SQk, char * SVj, char * SVk, int Sinst_index=0):
        name(Sname), time(Stime), busy(Sbusy), op(Sop), Qj(SQj), Qk(SQk), Vj(SVj), Vk(SVk), inst_idex(Sinst_index){}
};

struct Load{
    char * name;
    int time;
    bool busy;
    char * address;
    Load(char * Sname, int Stime, bool Sbusy, char * Saddress):
        name(Sname), time(Stime), busy(Sbusy), address(Saddress){}
};


struct inst{
    char * op;
    int rs, rt, rd;
    int state;
    int RS_index;
    inst(char * opcode, int regd, int regs, int regt, int prev_state=unready, int MRS_index=0):
        op(opcode), rs(regs), rt(regt), rd(regd), state(prev_state), RS_index(MRS_index){}
};


vector<inst> insts;
vector<RS> RSs;
vector<Load> Loads;
char * regstatus[30];
char * regvalue[30];

int inst_state[200][3];
int my_clock = 0;

int exec_time[6] = {2, 2, 10, 2, 2, 40};
int reg_count = 20;

char *functs[9] = {"Load1", "Load2", "Load3", "Add1", "Add2", "Add3", "Mult1", "Mult2", ""};
char *opCode[7] = {"LOAD", "STORE", "MUL", "SUB", "ADD", "DIV", ""};
char *tmp_buffer = new char[1000];
char *reg_buffer = new char[1000];
int buffer_ptr = 0;
int reg_ptr = 0;

char * get_regvalue(int idx){
    if(regvalue[idx] == nullptr){
        sprintf(reg_buffer + reg_ptr, "F%-2d", idx);
        // printf("%s----------", reg_buffer+reg_ptr);
        char * ret = reg_buffer + reg_ptr;
        reg_ptr += 4;
        return ret;
    }
    else{
        return regvalue[idx];
    }
}
// 将保留站、LoadBuffer、寄存器状态、指令状态拷贝一份，需要在next()进入的时候调用
// 保存上次状态的数据结构：
vector<RS> RSs_old;
vector<Load> Loads_old;
char * regstatus_old[30];
int inst_state_old[200][3];

void save_old(){
    RSs_old.clear();
    Loads_old.clear();
    for(int i = 0; i < RSs.size(); i++){
        RSs_old.push_back(RS(RSs[i].name, RSs[i].time, RSs[i].busy, RSs[i].op, RSs[i].Qj, RSs[i].Qk, RSs[i].Vj, RSs[i].Vk, RSs[i].inst_idex));
    }
    for(int i = 0; i < Loads.size(); i++){
        Loads_old.push_back(Load(Loads[i].name, Loads[i].time, Loads[i].busy, Loads[i].address));
    }
    for(int i = 0; i < 30; i++){
        regstatus_old[i] = regstatus[i];
    }
    for(int i = 0; i < insts.size(); i++){
        for(int j = 0; j < 3; j++){
            inst_state_old[i][j] = inst_state[i][j];
        }
    }
}

bool next(){
    save_old();
    bool RS_hit[10] = {false};
    bool Load_hit[3] = {false};
    bool issue_flag = false;
    for(int i = 0; i < insts.size(); i++){
        auto & cur = insts[i];
        if(cur.state == unready && !issue_flag){ // 尝试流出
            if(cur.op == opCode[0] || cur.op == opCode[1]){ // Load or Store
                for(int j = 0; j < Loads.size(); j++){
                    auto & cur_load = Loads[j];
                    if(!cur_load.busy && !Load_hit[j]){
                        Load_hit[j] = true;
                        inst_state[i][0] = my_clock;
                        cur.state = issue;
                        cur_load.busy = true;
                        cur_load.time = exec_time[0]; // Load for 2 cycles
                        cur.RS_index = j; 
                        // wirte "34+R2" to Load1, then change the buffer ptr
                        sprintf(tmp_buffer + buffer_ptr, "%d+R%d", cur.rs, cur.rt);
                        cur_load.address = tmp_buffer + buffer_ptr;
                        buffer_ptr += 10;
                        regstatus[cur.rd] = cur_load.name;
                        issue_flag = true;
                        break;
                    }
                }
            }
            else if(cur.op == opCode[2] || cur.op == opCode[5]){ // MUL, DIV
                for(int j = 0; j < RSs.size(); j++){
                    auto & cur_RS = RSs[j];
                    if(!cur_RS.busy && (cur_RS.name == functs[6] || cur_RS.name == functs[7]) && !RS_hit[j]){
                        RS_hit[j] = true;
                        cur.RS_index = j;
                        inst_state[i][0] = my_clock;
                        cur.state = issue;
                        cur_RS.busy = true;
                        cur_RS.op = cur.op;
                        cur_RS.inst_idex = i;
                        if(regstatus[cur.rs] == functs[8])
                            cur_RS.Vj = get_regvalue(cur.rs);
                        else
                            cur_RS.Qj = regstatus[cur.rs];
                        if(regstatus[cur.rt] == functs[8])
                            cur_RS.Vk = get_regvalue(cur.rt);
                        else
                            cur_RS.Qk = regstatus[cur.rt];
                        if(cur_RS.Qj == nullptr && cur_RS.Qk == nullptr){
                            if(cur.op == opCode[2]) cur_RS.time = exec_time[2];
                            else cur_RS.time = exec_time[5];
                        }
                        regstatus[cur.rd] = cur_RS.name;
                        issue_flag = true;
                        break;
                    }
                }
            }
            else if(cur.op == opCode[3] || cur.op == opCode[4]){ // SUB, ADD
                for(int j = 0; j < RSs.size(); j++){
                    auto & cur_RS = RSs[j];
                    if(!cur_RS.busy && (cur_RS.name == functs[3] || cur_RS.name == functs[4] || cur_RS.name == functs[5]) && !RS_hit[j]){\
                        RS_hit[j] = true;
                        inst_state[i][0] = my_clock;
                        cur.RS_index = j;
                        cur.state = issue;
                        cur_RS.busy = true;
                        cur_RS.op = cur.op;
                        cur_RS.inst_idex = i;
                        if(regstatus[cur.rs] == functs[8])
                            cur_RS.Vj = get_regvalue(cur.rs);
                        else
                            cur_RS.Qj = regstatus[cur.rs];
                        if(regstatus[cur.rt] == functs[8])
                            cur_RS.Vk = get_regvalue(cur.rt);
                        else
                            cur_RS.Qk = regstatus[cur.rt];
                        if(cur_RS.Qj == nullptr && cur_RS.Qk == nullptr){
                            if(cur.op == opCode[3]) cur_RS.time = exec_time[3];
                            else cur_RS.time = exec_time[4];
                        }
                        regstatus[cur.rd] = cur_RS.name;
                        issue_flag = true;
                        break;
                    }
                }
            }
        }
        if(cur.state == issue){ // 尝试执行
            if(!Load_hit[cur.RS_index] && (cur.op == opCode[0] || cur.op == opCode[1])){ // Load or Store
                auto & cur_load = Loads[cur.RS_index];
                Load_hit[cur.RS_index] = true;
                cur_load.time -= 1;
                if(cur_load.time == 0){
                    cur.state = execmpl;
                    inst_state[i][1] = my_clock;
                }
            }
            else if(!RS_hit[cur.RS_index] && (cur.op == opCode[2] || cur.op == opCode[3] || cur.op == opCode[4] || cur.op == opCode[5])){ // MUL, DIV, SUB, ADD
                auto & cur_RS = RSs[cur.RS_index];
                if(cur_RS.Qj == nullptr && cur_RS.Qk == nullptr){
                    RS_hit[cur.RS_index] = true;
                    cur_RS.time -= 1;
                    if(cur_RS.time == 0){
                        cur.state = execmpl;
                        inst_state[i][1] = my_clock;
                    }
                }
            }
        }
        if(cur.state == execmpl){ // 尝试写回
            if(!Load_hit[cur.RS_index] && (cur.op == opCode[0] || cur.op == opCode[1])){ // Load or Store
                auto & cur_load = Loads[cur.RS_index];
                Load_hit[cur.RS_index] = true;
                // 写入REG和需要这个数字的保留站
                sprintf(tmp_buffer + buffer_ptr, "M[%s]", cur_load.address);
                regvalue[cur.rd] = tmp_buffer + buffer_ptr;
                for (int j = 0; j < RSs.size(); j++){
                    auto & cur_RS = RSs[j];
                    if(cur_RS.Qj == cur_load.name){
                        cur_RS.Vj = regvalue[cur.rd];
                        cur_RS.Qj = nullptr;
                        RS_hit[j] = true;
                        if (cur_RS.op == opCode[2]) cur_RS.time = exec_time[2];
                        else if (cur_RS.op == opCode[5]) cur_RS.time = exec_time[5];
                        else if (cur_RS.op == opCode[3]) cur_RS.time = exec_time[3];
                        else if (cur_RS.op == opCode[4]) cur_RS.time = exec_time[4];
                    }
                    if(cur_RS.Qk == cur_load.name){
                        cur_RS.Vk = regvalue[cur.rd];
                        cur_RS.Qk = nullptr;
                        RS_hit[j] = true;
                        if (cur_RS.op == opCode[2]) cur_RS.time = exec_time[2];
                        else if (cur_RS.op == opCode[5]) cur_RS.time = exec_time[5];
                        else if (cur_RS.op == opCode[3]) cur_RS.time = exec_time[3];
                        else if (cur_RS.op == opCode[4]) cur_RS.time = exec_time[4];
                    }
                }
                buffer_ptr += strlen(cur_load.address) + 4;
                regstatus[cur.rd] = functs[8];
                cur_load.busy = false;
                cur_load.address = nullptr;
                cur.state = writebk;
                inst_state[i][2] = my_clock;
            }
            else if(!RS_hit[cur.RS_index] && (cur.op == opCode[2] || cur.op == opCode[3] || cur.op == opCode[4] || cur.op == opCode[5])){ // MUL, DIV, SUB, ADD
                auto & cur_RS = RSs[cur.RS_index];
                RS_hit[cur.RS_index] = true;
                // 写入REG和需要这个数字的保留站
                if(cur_RS.op == opCode[2]) sprintf(tmp_buffer + buffer_ptr, "%s*%s", cur_RS.Vj, cur_RS.Vk);
                else if(cur_RS.op == opCode[3]) sprintf(tmp_buffer + buffer_ptr, "%s-%s", cur_RS.Vj, cur_RS.Vk);
                else if(cur_RS.op == opCode[4]) sprintf(tmp_buffer + buffer_ptr, "%s+%s", cur_RS.Vj, cur_RS.Vk);
                else if(cur_RS.op == opCode[5]) sprintf(tmp_buffer + buffer_ptr, "%s/%s", cur_RS.Vj, cur_RS.Vk);
                regvalue[cur.rd] = tmp_buffer + buffer_ptr;
                for (int j = 0; j < RSs.size(); j++){
                    auto & cur_RS_j = RSs[j];
                    if(cur_RS_j.Qj == cur_RS.name){
                        cur_RS_j.Vj = regvalue[cur.rd];
                        cur_RS_j.Qj = nullptr;
                        RS_hit[j] = true;
                        if(cur_RS_j.op == opCode[2]) cur_RS_j.time = exec_time[2];
                        else if(cur_RS_j.op == opCode[5]) cur_RS_j.time = exec_time[5];
                        else if(cur_RS_j.op == opCode[3]) cur_RS_j.time = exec_time[3];
                        else if(cur_RS_j.op == opCode[4]) cur_RS_j.time = exec_time[4];
                    }
                    if(cur_RS_j.Qk == cur_RS.name){
                        cur_RS_j.Vk = regvalue[cur.rd];
                        cur_RS_j.Qk = nullptr;
                        RS_hit[j] = true;
                        if(cur_RS_j.op == opCode[2]) cur_RS_j.time = exec_time[2];
                        else if(cur_RS_j.op == opCode[5]) cur_RS_j.time = exec_time[5];
                        else if(cur_RS_j.op == opCode[3]) cur_RS_j.time = exec_time[3];
                        else if(cur_RS_j.op == opCode[4]) cur_RS_j.time = exec_time[4];
                    }
                }
                buffer_ptr += strlen(cur_RS.Vj) + strlen(cur_RS.Vk) + 2;
                regstatus[cur.rd] = functs[8];
                cur_RS.busy = false;
                cur_RS.Vj = nullptr;
                cur_RS.Vk = nullptr;
                cur_RS.op = opCode[6];
                cur.state = writebk;
                inst_state[i][2] = my_clock;
            }
        }
    }
    bool ret = false;
    for(int i = 0; i < insts.size(); i++){
        if(insts[i].state != writebk){
            ret = true;
            break;
        }
    }
    return ret;
}

// 支持old参数的print函数，用于打印上一次的状态

void printRS(bool old=false){
    // printf("Time Name      Busy    Op       Qj      Qk      Vj                  Vk\n");
    // for(int i = 0; i < RSs.size(); i++){
    //     printf("%-5d", RSs[i].time);
    //     printf("%-10s", RSs[i].name);
    //     if(RSs[i].busy) printf("Yes     ");
    //     else printf("No      ");
    //     printf("%-8s ", RSs[i].op);
    //     if(RSs[i].Qj == nullptr) printf("        ");
    //     else printf("%-7s ", RSs[i].Qj);
    //     if(RSs[i].Qk == nullptr) printf("        ");
    //     else printf("%-7s ", RSs[i].Qk);
    //     if(RSs[i].Vj == nullptr) printf("                    ");
    //     else printf("%-19s ", RSs[i].Vj);
    //     if(RSs[i].Vk == nullptr) printf("                    ");
    //     else printf("%-19s ", RSs[i].Vk);
    //     puts("");
    // }
    if(old){
        printf("Time Name      Busy    Op       Qj      Qk      Vj                  Vk\n");
        for(int i = 0; i < RSs_old.size(); i++){
            printf("%-5d", RSs_old[i].time);
            printf("%-10s", RSs_old[i].name);
            if(RSs_old[i].busy) printf("Yes     ");
            else printf("No      ");
            printf("%-8s ", RSs_old[i].op);
            if(RSs_old[i].Qj == nullptr) printf("        ");
            else printf("%-7s ", RSs_old[i].Qj);
            if(RSs_old[i].Qk == nullptr) printf("        ");
            else printf("%-7s ", RSs_old[i].Qk);
            if(RSs_old[i].Vj == nullptr) printf("                    ");
            else printf("%-19s ", RSs_old[i].Vj);
            if(RSs_old[i].Vk == nullptr) printf("                    ");
            else printf("%-19s ", RSs_old[i].Vk);
            puts("");
        }
    }
    else{
        printf("Time Name      Busy    Op       Qj      Qk      Vj                  Vk\n");
        for(int i = 0; i < RSs.size(); i++){
            printf("%-5d", RSs[i].time);
            printf("%-10s", RSs[i].name);
            if(RSs[i].busy) printf("Yes     ");
            else printf("No      ");
            printf("%-8s ", RSs[i].op);
            if(RSs[i].Qj == nullptr) printf("        ");
            else printf("%-7s ", RSs[i].Qj);
            if(RSs[i].Qk == nullptr) printf("        ");
            else printf("%-7s ", RSs[i].Qk);
            if(RSs[i].Vj == nullptr) printf("                    ");
            else printf("%-19s ", RSs[i].Vj);
            if(RSs[i].Vk == nullptr) printf("                    ");
            else printf("%-19s ", RSs[i].Vk);
            puts("");
        }
    }
}

void printLoad(bool old=false){
    // printf("Time Name      Busy    Address\n");
    // for(int i = 0; i < Loads.size(); i++){
    //     printf("%-5d", Loads[i].time);
    //     printf("%-10s", Loads[i].name);
    //     if(Loads[i].busy) printf("Yes ");
    //     else printf("No  ");
    //     if(Loads[i].address == nullptr) printf("        ");
    //     else printf("%9s ", Loads[i].address);
    //     puts("");
    // }
    if(old){
        printf("Time Name      Busy    Address\n");
        for(int i = 0; i < Loads_old.size(); i++){
            printf("%-5d", Loads_old[i].time);
            printf("%-10s", Loads_old[i].name);
            if(Loads_old[i].busy) printf("Yes ");
            else printf("No  ");
            if(Loads_old[i].address == nullptr) printf("        ");
            else printf("%9s ", Loads_old[i].address);
            puts("");
        }
    }
    else{
        printf("Time Name      Busy    Address\n");
        for(int i = 0; i < Loads.size(); i++){
            printf("%-5d", Loads[i].time);
            printf("%-10s", Loads[i].name);
            if(Loads[i].busy) printf("Yes ");
            else printf("No  ");
            if(Loads[i].address == nullptr) printf("        ");
            else printf("%9s ", Loads[i].address);
            puts("");
        }
    }
}

void printInst(bool old=false){
    // for(int i = 0; i < insts.size(); i++){
    //     printf("%-6s", insts[i].op);
    //     if(insts[i].op == "LOAD"){
    //         printf("F%-2d ", insts[i].rd);
    //         printf("%-3d ", insts[i].rs);
    //         printf("R%-2d    ", insts[i].rt);
    //         for(int k = 0; k < 3; k++){
    //             printf("%3d", inst_state[i][k]);
    //         }
    //         puts("");
    //     }
    //     else{
    //         printf("F%-2d ", insts[i].rd);
    //         printf("F%-2d ", insts[i].rs);
    //         printf("F%-2d    ", insts[i].rt);
    //         for(int k = 0; k < 3; k++){
    //             printf("%3d", inst_state[i][k]);
    //         }
    //         puts("");
    //     }
    // }
    if(old){
        for(int i = 0; i < insts.size(); i++){
            printf("%-6s", insts[i].op);
            if(insts[i].op == "LOAD"){
                printf("F%-2d ", insts[i].rd);
                printf("%-3d ", insts[i].rs);
                printf("R%-2d    ", insts[i].rt);
                for(int k = 0; k < 3; k++){
                    printf("%3d", inst_state_old[i][k]);
                }
                puts("");
            }
            else{
                printf("F%-2d ", insts[i].rd);
                printf("F%-2d ", insts[i].rs);
                printf("F%-2d    ", insts[i].rt);
                for(int k = 0; k < 3; k++){
                    printf("%3d", inst_state_old[i][k]);
                }
                puts("");
            }
        }
    }
    else{
        for(int i = 0; i < insts.size(); i++){
            printf("%-6s", insts[i].op);
            if(insts[i].op == "LOAD"){
                printf("F%-2d ", insts[i].rd);
                printf("%-3d ", insts[i].rs);
                printf("R%-2d    ", insts[i].rt);
                for(int k = 0; k < 3; k++){
                    printf("%3d", inst_state[i][k]);
                }
                puts("");
            }
            else{
                printf("F%-2d ", insts[i].rd);
                printf("F%-2d ", insts[i].rs);
                printf("F%-2d    ", insts[i].rt);
                for(int k = 0; k < 3; k++){
                    printf("%3d", inst_state[i][k]);
                }
                puts("");
            }
        }
    }
}

void printRegs(bool old=false){
    // for(int i = 0; i < reg_count; i+=2){
    //     printf("F%-10d", i);
    // }
    // puts("");
    // for(int i = 0; i < reg_count; i+=2){
    //     printf("%-10s ", regstatus[i]);
    // }
    // puts("");
    
    if(old){
        for(int i = 0; i < reg_count; i+=2){
            printf("F%-10d", i);
        }
        puts("");
        for(int i = 0; i < reg_count; i+=2){
            printf("%-10s ", regstatus_old[i]);
        }
        puts("");
    }
    else{
        for(int i = 0; i < reg_count; i+=2){
            printf("F%-10d", i);
        }
        puts("");
        for(int i = 0; i < reg_count; i+=2){
            printf("%-10s ", regstatus[i]);
        }
        puts("");
    }
}

void printmy(bool old=false){
    if(old){
        printInst(true);
        puts("");
        printRS(true);
        puts("");
        printLoad(true);
        puts("");
        printRegs(true);
        puts("");
    }
    else{
        printInst();
        puts("");
        printRS();
        puts("");
        printLoad();
        puts("");
        printRegs();
        puts("");
    }
}

int main(){
    for(int i = 0; i < 30; i++){
        // regstatus[i] = nullptr;
        regstatus[i] = functs[8];
        regvalue[i] = nullptr;
    }
    Loads.push_back(Load(functs[0], 0, false, nullptr));
    Loads.push_back(Load(functs[1], 0, false, nullptr));
    Loads.push_back(Load(functs[2], 0, false, nullptr));
    RSs.push_back(RS(functs[3], 0, false, opCode[6], nullptr, nullptr, nullptr, nullptr));
    RSs.push_back(RS(functs[4], 0, false, opCode[6], nullptr, nullptr, nullptr, nullptr));
    RSs.push_back(RS(functs[5], 0, false, opCode[6], nullptr, nullptr, nullptr, nullptr));
    RSs.push_back(RS(functs[6], 0, false, opCode[6], nullptr, nullptr, nullptr, nullptr));
    RSs.push_back(RS(functs[7], 0, false, opCode[6], nullptr, nullptr, nullptr, nullptr));
    // insts.push_back(inst(opCode[0],  4, 34,  2));
    // insts.push_back(inst(opCode[0],  2, 45,  3));
    // insts.push_back(inst(opCode[4],  0,  2,  4));
    // insts.push_back(inst(opCode[3],  8,  6,  0));
    // insts.push_back(inst(opCode[3], 10,  0,  6));
    // insts.push_back(inst(opCode[4],  6,  8,  4));

    insts.push_back(inst(opCode[0],  10, 21,  3));
    insts.push_back(inst(opCode[2],  2, 16,  4));
    // insts.push_back(inst(opCode[2],  2,  4,  6));
    // insts.push_back(inst(opCode[3], 10,  8,  4));
    insts.push_back(inst(opCode[5], 12,  2,  8));
    insts.push_back(inst(opCode[4],  8, 10,  4));
    int ne = 1;
    int chk = 0;
    bool live = true;
    while(ne){
        while(ne--){
            my_clock += 1;
            live = next();
        }
        cout<<"Clock: "<<my_clock<<endl;
        printmy(0);
        if(!live){
            cout<<"All instructions have been executed."<<endl;
            break;
        }
        // cout<<"Want to check last status? 1 for yes, 0 for no: ";
        // cin>>chk;
        // if(chk){
        //     printmy(1);
        // }
        cout<<"1 for 1 cycle, n for n cycles: ";
        cin>>ne;
    }
    return 0;
}