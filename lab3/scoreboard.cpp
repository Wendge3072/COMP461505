#include<iostream>
#include<vector>
#include<string.h>
using namespace std;

enum state {unready = 4, issue = 0, readop = 1, execmpl = 2, writebk = 3};

struct func{
    char * name;
    int time;
    bool busy;
    char * op;
    int Fi, Fj, Fk;
    char * Qj, * Qk;
    bool Rj, Rk;
    int inst_idex;
    func( char * Sname, int Stime, bool Sbusy, char * Sop, int SFi, int SFj, int SFk,
          char * SQj, char * SQk, bool SRj, bool SRk, int Sinst_index=0):
        name(Sname), time(Stime), busy(Sbusy), op(Sop), Fi(SFi), Fj(SFj), Fk(SFk),
        Qj(SQj), Qk(SQk), Rj(SRj), Rk(SRk), inst_idex(Sinst_index){}
};

struct inst{
    char * op;
    int rs, rt, rd;
    int state;
    int func_index;
    inst(char * opcode, int regd, int regs, int regt, int prev_state=unready, int Mfunc_index=0):
        op(opcode), rs(regs), rt(regt), rd(regd), state(prev_state), func_index(Mfunc_index){}
};

vector<inst> insts;
vector<func> func_table;
char * regstatus[30];

int inst_state[200][4];
int my_clock = 0;

char *functs[6] = {"Integer", "Mult1", "Mult2", "Add", "Divide", ""};
char *opCode[7] = {"LOAD", "STORE", "MUL", "SUB", "ADD", "DIV", ""};

void next(){
    bool func_hit[10];
    memset(func_hit, 0, sizeof func_hit);
    bool issue_flag = false;
    for(int i = 0; i < insts.size(); i++){
        if(!issue_flag && insts[i].state == unready){ // 尝试流出
            auto & it = insts[i];
            if(regstatus[it.rd] == functs[5]){
                int target = -1;
                if(it.op == opCode[0]){
                    for(int j = 0; j < func_table.size(); j++){
                        // cout<<j<<func_table[j].name << func_table[j].busy<<endl;
                        if(func_table[j].name == functs[0] && !func_table[j].busy){
                            target = j;
                            break;
                        }
                    }
                }
                else if(it.op == opCode[2]){
                    for(int j = 0; j < func_table.size(); j++){
                        if((func_table[j].name == functs[1] || func_table[j].name == functs[2]) && !func_table[j].busy){
                            target = j;
                            break;
                        }
                    }
                }
                else if(it.op == opCode[5]){
                    for(int j = 0; j < func_table.size(); j++){
                        if(func_table[j].name == functs[4] && !func_table[j].busy){
                            target = j;
                            break;
                        }
                    }
                }
                else if(it.op == opCode[3] || it.op == opCode[4]){
                    for(int j = 0; j < func_table.size(); j++){
                        if(func_table[j].name == functs[3] && !func_table[j].busy){
                            target = j;
                            break;
                        }
                    }
                }
                if(target != -1 && !func_hit[target]){ // 可以流出
                    it.func_index = target;
                    func_table[target].busy = true;
                    func_table[target].Fi = it.rd;
                    func_table[target].Fj = it.rs;
                    func_table[target].Fk = it.rt;
                    func_table[target].op = it.op;
                    func_table[target].inst_idex = i;
                    if (it.op == opCode[0]){
                        func_table[target].Qj = functs[5];
                        func_table[target].Qk = functs[5];
                    }
                    else{
                        func_table[target].Qj = regstatus[it.rs];
                        func_table[target].Qk = regstatus[it.rt];
                    }
                    
                    func_table[target].Rj = (func_table[target].Qj == functs[5]);
                    func_table[target].Rk = (func_table[target].Qk == functs[5]);
                    regstatus[it.rd] = func_table[target].name;
                    inst_state[i][0] = my_clock;
                    issue_flag = true;
                    it.state = issue;
                    continue;
                }
                else issue_flag = true;
            }
            else issue_flag = true;
        }
        if(insts[i].state == issue){
            auto & it = insts[i];
            auto & ft = func_table[it.func_index];
            if(ft.Rj && ft.Rk && !func_hit[it.func_index]){
                ft.Rj = ft.Rk = false;
                func_hit[it.func_index] = true;
                inst_state[i][1] = my_clock;
                if(it.op == opCode[0]){
                    ft.time = 1;
                }
                else if(it.op == opCode[2]){
                    ft.time = 10;
                }
                else if(it.op == opCode[5]){
                    ft.time = 40;
                }
                else if(it.op == opCode[3] || it.op == opCode[4]){
                    ft.time = 2;
                }
                it.state = readop;
                continue;
            }
        }
        if(insts[i].state == readop){
            auto & it = insts[i];
            auto & ft = func_table[it.func_index];
            ft.time--;
            if(ft.time == 0){
                inst_state[i][2] = my_clock;
                it.state = execmpl;
                continue;
            }
        }
        if(insts[i].state == execmpl){
            auto & it = insts[i];
            auto & ft = func_table[it.func_index];
            if(func_hit[it.func_index]) continue;
            bool write_flag = true;
            for(int j = 0; j < func_table.size(); j++){
                if(func_table[j].name == functs[0]) continue;
                if(func_table[j].Fj == ft.Fi && func_table[j].Rj || func_hit[j]){
                    write_flag = false;
                    break;
                }
                if(func_table[j].Fk == ft.Fi && func_table[j].Rk || func_hit[j]){
                    write_flag = false;
                    break;
                }
            }
            if(write_flag){
                for(int j = 0; j < func_table.size(); j++){
                    if(func_table[j].Qj == ft.name){
                        func_table[j].Qj = functs[5];
                        func_table[j].Rj = true;
                        func_hit[j] = true;
                    }
                    if(func_table[j].Qk == ft.name){
                        func_table[j].Qk = functs[5];
                        func_table[j].Rk = true;
                        func_hit[j] = true;
                    }
                }
                regstatus[ft.Fi] = functs[5];
                ft.busy = false;
                ft.Fi = -1;
                ft.Fj = -1;
                ft.Fk = -1;
                ft.inst_idex = 0;
                ft.op = opCode[6];
                ft.Qj = functs[5];
                ft.Qk = functs[5];
                ft.Rj = ft.Rk = false;
                inst_state[i][3] = my_clock;
                func_hit[it.func_index] = true;
                it.state = writebk;
                continue;
            }
        }
    }
}


void printInst(){
    for(int i = 0; i < insts.size(); i++){
        printf("%-6s", insts[i].op);
        if(insts[i].op == "LOAD"){
            printf("F%-2d ", insts[i].rd);
            printf("%-3d ", insts[i].rs);
            printf("R%-2d    ", insts[i].rt);
            for(int k = 0; k < 4; k++){
                printf("%3d", inst_state[i][k]);
            }
            puts("");
        }
        else{
            printf("F%-2d ", insts[i].rd);
            printf("F%-2d ", insts[i].rs);
            printf("F%-2d    ", insts[i].rt);
            for(int k = 0; k < 4; k++){
                printf("%3d", inst_state[i][k]);
            }
            puts("");
        }
    }
}

void printFunc(){
    printf("Time Name      Busy    Op    Fi   Fj   Fk    Qj         Qk       Rj   Rk\n");
    for(int i = 0; i < func_table.size(); i++){
        printf("%-5d", func_table[i].time);
        printf("%-10s", func_table[i].name);
        if(func_table[i].busy){
            printf("Yes ");
        }
        else{
            printf("No  ");
        }
        printf("%8s ", func_table[i].op);
        if(func_table[i].name == "Integer"){
            printf(" F%-2d ", func_table[i].Fi);
            printf("     ");
            printf(" R%-2d ", func_table[i].Fk);
        }
        else{
            printf(" F%-2d ", func_table[i].Fi);
            printf(" F%-2d ", func_table[i].Fj);
            printf(" F%-2d ", func_table[i].Fk);
        }
        printf("%-10s ", func_table[i].Qj);
        printf("%-10s ", func_table[i].Qk);
        if(func_table[i].Rj){
            printf("Yes  ");
        }
        else{
            printf("No   ");
        }
        if(func_table[i].Rk){
            printf("Yes ");
        }
        else{
            printf("No  ");
        }
        printf("\n");
    }
}

void printRegs(){
    for(int i = 0; i < 30; i+=2){
        printf("F%-10d", i);
    }
    puts("");
    for(int i = 0; i < 30; i+=2){
        printf("%-10s ", regstatus[i]);
    }
    puts("");
}


int main(){
    for(int i = 0; i < 30; i++){
        regstatus[i] = functs[5];
    }
    func_table.push_back(func(functs[0], 0, false, opCode[6], -1, -1, -1, functs[5], functs[5], false, false));
    func_table.push_back(func(functs[1], 0, false, opCode[6], -1, -1, -1, functs[5], functs[5], false, false));
    func_table.push_back(func(functs[2], 0, false, opCode[6], -1, -1, -1, functs[5], functs[5], false, false));
    func_table.push_back(func(functs[3], 0, false, opCode[6], -1, -1, -1, functs[5], functs[5], false, false));
    func_table.push_back(func(functs[4], 0, false, opCode[6], -1, -1, -1, functs[5], functs[5], false, false));
    insts.push_back(inst(opCode[0],  6, 34,  2));
    insts.push_back(inst(opCode[0],  2, 45,  3));
    insts.push_back(inst(opCode[2],  0,  2,  4));
    insts.push_back(inst(opCode[3],  8,  6,  2));
    insts.push_back(inst(opCode[5], 10,  0,  6));
    insts.push_back(inst(opCode[4],  6,  8,  2));
    int clcs = 62;
    while(clcs--){
        my_clock += 1;
        next();
    }
    printInst();
    puts("");
    printFunc();
    puts("");
    printRegs();
}