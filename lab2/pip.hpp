#ifndef PIP_H
#define PIP_H
#include<iostream>
#include<string>
#include<vector>
using namespace std;

class Inst{
    public:
        string name;
        int rs, rt, rd, imm;
        int stage;  // 这里的stage不包括STALL，指代指令最新到达的阶段
        int issue_cycle = 0;
        Inst(string m_name, int m_rs, int m_rt, int m_td, int m_imm);
};

class status{
    public:
        int Inst_index;
        int stage;
        status(int m_Inst_index, int m_stage);
};

class pipline{
    private:
        vector<Inst> insts;
        int PC = 0;
        bool redirect = 0;
        vector<vector<status>> pip_log;
        int cycle = 1;
        int * reg;
        int * mem;
        int reg_num = 32;
        int mem_num = 1024;
        int n_stall_RAW = 0;
        int n_stall_branch = 0;
        int n_stall_struct = 0;
        int n_branch = 0;
        int branch_taken = 0;
        bool RAW(vector<status>& last, int op_reg, int index, bool redirect);
    public:
        void set_redirect();
        void insts_init(Inst & m_inst);
        void storages_init();
        void memory_set(int index, int value);
        void single_step();
        void multi_step(int n);
        void run_to_breakpoint();
        void run_to_the_end();
        void print_log();
        void print_mem(int end = 1024);
        void print_reg(int end = 32);
        void print_performance();
};

#endif