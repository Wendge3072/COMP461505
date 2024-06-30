#include "pip.hpp"

Inst::Inst(string m_name, int m_rs, int m_rt, int m_rd, int m_imm){
    name = m_name;
    rs = m_rs;
    rt = m_rt;
    rd = m_rd;
    imm = m_imm;
    stage = 0;
}

status::status(int m_Inst_index, int m_stage){
    Inst_index = m_Inst_index;
    stage = m_stage;
}
void pipline::set_redirect(){
    redirect = true;
}

// 0: UNISSUE, 1: IF, 2: ID, 3: ID_bad, 4: EX, 5: MEM, 6: WB, 7: FINISH, 8: STALL
void pipline::insts_init(Inst & minst){
    insts.push_back(minst);
    PC = 0;
}

void pipline::storages_init(){
    reg = new int[reg_num];
    mem = new int[mem_num];
    for(int i = 0; i < reg_num; i++)
        reg[i] = 0;
    for(int i = 0; i < mem_num; i++)
        mem[i] = 0;
}

void pipline::memory_set(int index, int value){
    mem[index] = value;
}

// 0: UNISSUE, 1: IF, 2: ID, 3: ID_bad, 4: EX, 5: MEM, 6: WB, 7: FINISH, 8: STALL
// RAW: read after write, 返回true表示有RAW
bool pipline::RAW(vector<status> & last, int op_reg, int index, bool redirect){
    int res = false;
    int barrier = redirect ? 4 : 6; // 若无定向，则pre_log是WB阶段之前的指令都不能读取；若有定向，则pre_log是EX阶段之前的指令不能读取， 3是EX，5是MEM
    for(int i = index - 1; i >= 0; i--){
        Inst inst = insts[last[i].Inst_index];
        if(inst.name == "ADDU" && inst.rt == op_reg && inst.stage < barrier ||
           inst.name == "LW"   && inst.rt == op_reg && inst.stage < barrier ||
           inst.name == "ADD"  && inst.rd == op_reg && inst.stage < barrier){
            res = true;
            break;
        }
    }
    return res;
}
// 0: UNISSUE, 1: IF, 2: ID, 3: ID_bad, 4: EX, 5: MEM, 6: WB, 7: FINISH, 8: STALL
void pipline::single_step(){
    vector<bool> occupied(5, false); // 0: IF, 1: ID, 2: EX, 3: MEM, 4: WB
    vector<status> new_log;
    bool stall = false;
    bool new_inst = true;
    if(!pip_log.empty()){
        auto last = pip_log.back();
        for(int i = 0; i < last.size(); i++){
            int inst_stage = insts[last[i].Inst_index].stage;
            if(stall){ // 如果前面有指令stall，那么后面的指令都要stall
                new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                continue;
            }
            if(inst_stage == 1){ // IF -> ID
                if(occupied[1]){
                    new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                    stall = true;
                    n_stall_struct++;
                }
                else{
                    Inst inst = insts[last[i].Inst_index];
                    if(inst.name == "ADDU"){
                        int op_reg = inst.rs;
                        bool raw = RAW(last, op_reg, i, redirect);
                        if(raw){
                            new_log.push_back(status(last[i].Inst_index, 3)); // ID_bad
                            insts[last[i].Inst_index].stage = 3;
                            n_stall_RAW++;
                        }
                        else{
                            new_log.push_back(status(last[i].Inst_index, 2)); // ID
                            insts[last[i].Inst_index].stage = 2;
                        }
                        occupied[1] = true;
                    }
                    if(inst.name == "ADD"){
                        int op_reg1 = inst.rs;
                        int op_reg2 = inst.rt;
                        bool raw1 = RAW(last, op_reg1, i, redirect), raw2 = RAW(last, op_reg2, i, redirect);
                        if(raw1 || raw2){
                            new_log.push_back(status(last[i].Inst_index, 3)); // ID_bad
                            insts[last[i].Inst_index].stage = 3;
                            n_stall_RAW++;
                        }
                        else{
                            new_log.push_back(status(last[i].Inst_index, 2)); // ID
                            insts[last[i].Inst_index].stage = 2;
                        }
                        occupied[1] = true;
                    }
                    if(inst.name == "LW"){
                        int op_reg = inst.rs;
                        bool raw = RAW(last, op_reg, i, redirect);
                        if(raw){
                            new_log.push_back(status(last[i].Inst_index, 3)); // ID_bad
                            insts[last[i].Inst_index].stage = 3;
                            n_stall_RAW++;
                        }
                        else{
                            new_log.push_back(status(last[i].Inst_index, 2)); // ID
                            insts[last[i].Inst_index].stage = 2;
                        }
                        occupied[1] = true;
                    }
                    if(inst.name == "BEQZ"){
                        int op_reg = inst.rs;
                        bool raw = RAW(last, op_reg, i, redirect);
                        if(raw){
                            new_log.push_back(status(last[i].Inst_index, 3)); // ID_bad
                            insts[last[i].Inst_index].stage = 3;
                            n_stall_RAW++;
                        }
                        else{
                            new_log.push_back(status(last[i].Inst_index, 2)); // ID
                            insts[last[i].Inst_index].stage = 2;
                            if(reg[inst.rs] == 0){
                                PC += inst.imm;
                                branch_taken++;
                            }
                            new_inst = false;
                            n_stall_branch++;
                        }
                        occupied[1] = true;
                    }
                    if(inst.name == "SW"){
                        int op_reg1 = inst.rs;
                        int op_reg2 = inst.rt;
                        bool raw1 = RAW(last, op_reg1, i, redirect), raw2 = RAW(last, op_reg2, i, redirect);
                        if(raw1 || raw2){
                            new_log.push_back(status(last[i].Inst_index, 3)); // ID_bad
                            insts[last[i].Inst_index].stage = 3;
                            n_stall_RAW++;
                        }
                        else{
                            new_log.push_back(status(last[i].Inst_index, 2)); // ID
                            insts[last[i].Inst_index].stage = 2;
                        }
                        occupied[1] = true;
                    }
                }
            }
            else if(inst_stage == 2){ // ID
                if(occupied[2]){
                    new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                    stall = true;
                    n_stall_struct++;
                }
                else{
                    new_log.push_back(status(last[i].Inst_index, 4)); // EX
                    insts[last[i].Inst_index].stage = 4;
                    occupied[2] = true;
                }
            }
            else if(inst_stage == 3){ // ID_bad 尝试ID，若无法ID，则STALL
                Inst inst = insts[last[i].Inst_index];
                if(inst.name == "ADDU"){
                    int op_reg = inst.rs;
                    bool raw = RAW(last, op_reg, i, redirect);
                    if(raw){
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_RAW++;
                    }
                    else if(!occupied[1]){
                        new_log.push_back(status(last[i].Inst_index, 2)); // ID
                        insts[last[i].Inst_index].stage = 2;
                        occupied[1] = true;
                    }
                    else{
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_struct++;
                    }
                }
                if(inst.name == "ADD"){
                    int op_reg1 = inst.rs;
                    int op_reg2 = inst.rt;
                    bool raw1 = RAW(last, op_reg1, i, redirect), raw2 = RAW(last, op_reg2, i, redirect);
                    if(raw1 || raw2){
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_RAW++;
                    }
                    else if(!occupied[1]){
                        new_log.push_back(status(last[i].Inst_index, 2)); // ID
                        insts[last[i].Inst_index].stage = 2;
                        occupied[1] = true;
                    }
                    else{
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_struct++;
                    }
                }
                if(inst.name == "LW"){
                    int op_reg = inst.rs;
                    bool raw = RAW(last, op_reg, i, redirect);
                    if(raw){
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_RAW++;
                    }
                    else if(!occupied[1]){
                        new_log.push_back(status(last[i].Inst_index, 2)); // ID
                        insts[last[i].Inst_index].stage = 2;
                        occupied[1] = true;
                    }
                    else{
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_struct++;
                    }
                }
                if(inst.name == "BEQZ"){
                    int op_reg = inst.rs;
                    bool raw = RAW(last, op_reg, i, redirect);
                    if(raw){
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_RAW++;
                    }
                    else if(!occupied[1]){
                        new_log.push_back(status(last[i].Inst_index, 2)); // ID
                        insts[last[i].Inst_index].stage = 2;
                        if(reg[inst.rs] == 0){
                            PC += inst.imm;
                            branch_taken++;
                        }
                        new_inst = false;
                        n_stall_branch++;
                        occupied[1] = true; 
                    }
                    else{
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_struct++;
                    }
                }
                if(inst.name == "SW"){
                    int op_reg1 = inst.rs;
                    int op_reg2 = inst.rt;
                    bool raw1 = RAW(last, op_reg1, i, redirect), raw2 = RAW(last, op_reg2, i, redirect);
                    if(raw1 || raw2){
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_RAW++;
                    }
                    else if(!occupied[1]){
                        new_log.push_back(status(last[i].Inst_index, 2)); // ID
                        insts[last[i].Inst_index].stage = 2;
                        occupied[1] = true;
                    }
                    else{
                        new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                        stall = true;
                        n_stall_struct++;
                    }
                }
            }
            else if(inst_stage == 4){ // EX
                if(occupied[3]){
                    new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                    stall = true;
                    n_stall_struct++;
                }
                else{
                    Inst inst = insts[last[i].Inst_index];
                    if(inst.name == "SW"){
                        int addr = reg[inst.rs] + inst.imm;
                        mem[addr] = reg[inst.rt];
                    }
                    new_log.push_back(status(last[i].Inst_index, 5)); // MEM
                    insts[last[i].Inst_index].stage = 5;
                    occupied[3] = true;
                }
            }
            else if(inst_stage == 5){ // MEM
                if(occupied[4]){
                    new_log.push_back(status(last[i].Inst_index, 8)); // STALL
                    stall = true;
                    n_stall_struct++;
                }
                else{
                    Inst inst = insts[last[i].Inst_index];
                    if(inst.name == "ADDU"){
                        reg[inst.rt] = reg[inst.rs] + inst.imm;
                    }
                    if(inst.name == "ADD"){
                        reg[inst.rd] = reg[inst.rs] + reg[inst.rt];
                    }
                    if(inst.name == "LW"){
                        int addr = reg[inst.rs] + inst.imm;
                        reg[inst.rt] = mem[addr];
                    }
                    new_log.push_back(status(last[i].Inst_index, 6)); // WB
                    insts[last[i].Inst_index].stage = 6;
                    occupied[4] = true;
                }
            }
            else if(inst_stage == 6){ // WB
                insts[last[i].Inst_index].stage = 7; // FINISH
                if(PC == insts.size() && last[i].Inst_index == PC - 1){
                    return;
                }
            }
        }
    }
    // issue one new instruction
    if(!stall && new_inst && PC < insts.size()){
        new_log.push_back(status(PC, 1));
        if(insts[PC].name == "BEQZ"){
            n_branch++;
        }
        insts[PC].stage = 1;
        insts[PC].issue_cycle = cycle;
        PC++;
    }
    pip_log.push_back(new_log);
    cycle++;
}
// 0: UNISSUE, 1: IF, 2: ID, 3: ID_bad, 4: EX, 5: MEM, 6: WB, 7: FINISH, 8: STALL

void pipline::multi_step(int n){
    while(n--){
        single_step();
    }
}

void pipline::run_to_the_end(){
    while(insts.back().stage != 6){
        single_step();
    }
}

void pipline::run_to_breakpoint(){
    int index;
    string stage;
    printf("请插入断点\n");
    printf("请输入断点指令序号和阶段\n");
    printf("指令序号: ");
    while(true){
        scanf("%d", &index);
        if(index >= insts.size()){
            printf("指令序号超出范围，请重新输入\n");
            printf("指令序号: ");
            continue;
        }
        break;
    }
    printf("阶段: ");
    cin >> stage;
    int stage_num;
    while(true){
        if(stage == "IF")
            stage_num = 1;
        else if(stage == "ID")
            stage_num = 2;
        else if(stage == "EX")
            stage_num = 4;
        else if(stage == "MEM")
            stage_num = 5;
        else if(stage == "WB")
            stage_num = 6;
        else {
            printf("阶段输入错误，请重新输入\n");
            printf("阶段: ");
            cin >> stage;
            continue;
        }
        break;
    }
    while(insts[index].stage < stage_num){
        single_step();
        if(insts[index].stage >= stage_num)
            break;
    }
}

void pipline::print_reg(int end){
    for(int i = 0; i < end; i+=8){
        for(int j = 0; j < 8; j++){
            printf("REG%7d  ", i + j);
        }
        printf("\n");
        for(int j = 0; j < 8; j++){
            printf("0x%08x  ", reg[i + j]);
        }
        printf("\n\n");
    }
}

void pipline::print_mem(int end){
    for(int i = 0; i < end; i+=8){
        printf("MEM%-4d ", i);
        for(int j = 0; j < 8; j++){
            printf("0x%08x  ", mem[i + j]);
        }
        printf("\n\n");
    }
}

void pipline::print_log(){
    int n_inst = insts.size(); 
    for(int i = 0; i < n_inst; i++){
        string name = insts[i].name;
        if(name == "ADDU")
            printf("Inst %d:    %4s $%d, $%d, %d", i, insts[i].name.c_str(), insts[i].rt, insts[i].rs, insts[i].imm);
        else if(name == "LW" || name == "SW")
            printf("Inst %d:    %4s $%d, %d($%d)", i, insts[i].name.c_str(), insts[i].rt, insts[i].imm, insts[i].rs);
        else if(name == "BEQZ")
            printf("Inst %d:    %4s $%d, %d", i, insts[i].name.c_str(), insts[i].rs, insts[i].imm);
        else if(name == "ADD")
            printf("Inst %d:    %4s $%d, $%d, $%d", i, insts[i].name.c_str(), insts[i].rd, insts[i].rs, insts[i].rt);
        printf("\n");
    }
    printf("-------------------------------\n");
    for(int i = 0; i < n_inst; i++){
        printf("Inst %d:", i);
        int n_space = insts[i].issue_cycle - 1;
        n_space = max(n_space, 0);
        for(int j = 0; j < n_space; j++)
            printf("      ");
        for(auto log : pip_log){
            for(auto s : log){
                if(s.Inst_index == i){
                    int stage = s.stage;
                    if(stage == 1)
                        printf(" %5s", "IF");
                    else if(stage == 2)
                        printf(" %5s", "ID");
                    else if(stage == 3)
                        printf(" %5s", "ID*");
                    else if(stage == 4)
                        printf(" %5s", "EX");
                    else if(stage == 5)
                        printf(" %5s", "MEM");
                    else if(stage == 6)
                        printf(" %5s", "WB");
                    else if(stage == 8)
                        printf(" %5s", "STALL");
                    break;
                }
            }
        }
        printf("\n");
    }
}

void pipline::print_performance(){
    // 周期数、定向机制、RAW停顿、控制停顿、分支指令条数、成功与失败占比、load store指令条数、占比  
    printf("Cycle_index: %d\n", cycle - 1);
    printf("Redirection: ");
    if(redirect) printf("ON");
    else printf("OFF");
    printf("\n");
    printf("RAW_stall: %d\n", n_stall_RAW);
    printf("Control_stall: %d\n", n_stall_branch);
    printf("Struction_stall: %d\n", n_stall_struct);
    printf("Branch_inst: %d\n", n_branch);
    printf("Branch_success_rate: %f\n", (float)(branch_taken) / n_branch);
}