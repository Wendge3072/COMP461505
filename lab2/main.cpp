#include "pip.hpp"
using namespace std;
void model1();
void model2();
void model3();
void model4();
int main(){
    // addu rt, rs, imm         read rs,      write rt
    // add  rd, rs, rt          read rs, rt   write rd
    // lw   rt, rs, imm         read rs,      write rt
    // sw   rt, rs, imm         read rs, rt
    // beqz rs, imm             read rs
    model2();
    return 0;
}

void model1(){
    // ADDU  R1, R0, 32
    // ADDU  R7, R7, 1
    // ADDU  R8, R8, 1
    // LW    R3, 0(R1)
    // LW    R4, 0(R1)
    // Lw    R5, 0(R1)
    // ADDU  R3, R3, 1111
    // SW    R4, 0(R1)
    vector<Inst> inst;
    inst.push_back(Inst("ADDU", 0, 1, 0, 32));
    inst.push_back(Inst("ADDU", 7, 7, 0, 1));
    inst.push_back(Inst("ADDU", 8, 8, 0, 1));
    inst.push_back(Inst("LW"  , 1, 3, 0, 0));
    inst.push_back(Inst("LW"  , 1, 4, 0, 0));
    inst.push_back(Inst("LW"  , 1, 5, 0, 0));
    inst.push_back(Inst("ADDU", 3, 3, 0, 1111));
    inst.push_back(Inst("SW"  , 1, 4, 0, 0));
    pipline p;
    p.storages_init();
    for(int i = 0; i < inst.size(); i++){
        p.insts_init(inst[i]);
    }
    p.memory_set(32, 3172);
    // p.run_to_the_end();
    p.run_to_breakpoint();
    p.print_log();
    puts("");   
    p.print_reg(16);
    puts("");
    p.print_mem(64);
    puts("");
    p.print_performance();
}

void model2(){
    // ADDU  R1, R0, 32
    // LW    R3, 0(R1)
    // LW    R4, 0(R1)
    // ADDU  R3, R3, 1111
    // SW    R3, 0(R1)
    vector<Inst> inst;
    inst.push_back(Inst("ADDU", 0, 1, 0, 32));
    inst.push_back(Inst("LW"  , 1, 3, 0, 0));
    inst.push_back(Inst("LW"  , 1, 4, 0, 0));
    inst.push_back(Inst("ADDU", 3, 3, 0, 1111));
    inst.push_back(Inst("SW"  , 1, 3, 0, 0));
    pipline p;
    p.storages_init();
    for(int i = 0; i < inst.size(); i++){
        p.insts_init(inst[i]);
    }
    p.memory_set(32, 3172);
    p.run_to_the_end();
    p.print_log();
    puts("");
    p.print_reg(16);
    puts("");
    p.print_mem(64);
    puts("");
    p.print_performance();
}

void model3(){
    // ADDU  R1, R0, 32
    // LW    R3, 0(R1)
    // LW    R4, 0(R1)
    // ADDU  R3, R3, 1111
    // SW    R3, 0(R1)
    vector<Inst> inst;
    inst.push_back(Inst("ADDU", 0, 1, 0, 32));
    inst.push_back(Inst("LW"  , 1, 3, 0, 0));
    inst.push_back(Inst("LW"  , 1, 4, 0, 0));
    inst.push_back(Inst("ADDU", 3, 3, 0, 1111));
    inst.push_back(Inst("SW"  , 1, 3, 0, 0));
    pipline p;
    p.storages_init();
    p.set_redirect();
    for(int i = 0; i < inst.size(); i++){
        p.insts_init(inst[i]);
    }
    p.memory_set(32, 3172);
    p.run_to_the_end();
    p.print_log();
    puts("");
    p.print_reg(16);
    puts("");
    p.print_mem(64);
    puts("");
    p.print_performance();
}

void model4(){
    // ADDU  R1, R0, 32
    // LW    R3, 0(R1)
    // LW    R4, 0(R1)
    // BEQZ  R5, 1
    // ADDU  R3, R3, 4444
    // ADDU  R3, R3, 1111
    // SW    R3, 0(R1)
    vector<Inst> inst;
    inst.push_back(Inst("ADDU", 0, 1, 0, 32));
    inst.push_back(Inst("LW"  , 1, 3, 0, 0));
    inst.push_back(Inst("LW"  , 1, 4, 0, 0));
    inst.push_back(Inst("BEQZ", 5, 0, 0, 1));
    inst.push_back(Inst("ADDU", 3, 3, 0, 4444));
    inst.push_back(Inst("ADDU", 3, 3, 0, 1111));
    inst.push_back(Inst("SW"  , 1, 3, 0, 0));
    pipline p;
    p.storages_init();
    // p.set_redirect();
    for(int i = 0; i < inst.size(); i++){
        p.insts_init(inst[i]);
    }
    p.memory_set(32, 3172);
    p.run_to_the_end();
    p.print_log();
    puts("");
    p.print_reg(16);
    puts("");
    p.print_mem(64);
    puts("");
    p.print_performance();
}