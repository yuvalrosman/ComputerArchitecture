/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <stdlib.h>

int** map;
int instructions_num_blocked;
int instructions_num_fg;
int blocked_cycles;
int fg_cycles;

int Next_RR(int** map,int* index, int curr_cycle,int threads_num)// find the next thread
{
	
	int start_index = *index;
	while(map[*index][8] == 0 || map[*index][9] >curr_cycle)
	{
		*index = (*index+1)%threads_num;
		if (*index == start_index) //  no avaliable threads
		{
			return -1;
		}
	}
	return -2;
}



void CORE_BlockedMT()
{
	int threads_num = SIM_GetThreadsNum();
	uint32_t* thread_current_instruction = malloc(sizeof(int)*threads_num); // keep track of each thread current inst
	for(int i=0; i<threads_num;i++)
	{	
		thread_current_instruction[i]=0;
	}
	int res=0;
	map = malloc(sizeof(int32_t*)*(threads_num));
	for(int i=0;i<threads_num;i++)
	{
		map[i] = malloc(sizeof(int32_t)*10); // 9 and 10 bit are to determine if the thread is active/inactive and in which cycle will i be ready
	}
	for(int i=0;i<threads_num;i++)
	{
		for(int j=0;j<8;j++)
		{
			map[i][j]=0;
		}
		map[i][8] = 1;
		map[i][9] = 0; 
	}
	int rr_counter =0; // the current thread running
	int active_threads = threads_num;
	int curr_cycle =0;
	Instruction* curr_instruction = malloc(sizeof(Instruction));
	while(active_threads>0)
	{	
		int tmp_rr = rr_counter;
		res = Next_RR(map,&rr_counter,curr_cycle,threads_num);
		if(res == -1) //no avaliable threads
		{
			curr_cycle++;
			continue;
		}
		if(tmp_rr != rr_counter)
		{
			curr_cycle+=SIM_GetSwitchCycles();
			continue; 
		}
		SIM_MemInstRead(thread_current_instruction[rr_counter],curr_instruction,rr_counter);
		while(curr_instruction->opcode!= CMD_LOAD && curr_instruction->opcode!= CMD_STORE && curr_instruction->opcode!= CMD_HALT)
		{
			instructions_num_blocked++;
			switch (curr_instruction->opcode) 
			{
			case CMD_LOAD:
				break;
			case CMD_STORE:
				break;
			case CMD_HALT:
				break;
        	case CMD_NOP: // NOP
            	break;
        	case CMD_ADDI:
				map[rr_counter][curr_instruction->dst_index] = 
				 	map[rr_counter][curr_instruction->src1_index] + 
					curr_instruction->src2_index_imm;
				thread_current_instruction[rr_counter]++;
				SIM_MemInstRead(thread_current_instruction[rr_counter],curr_instruction,rr_counter);
				break;
        	case CMD_SUBI:
            	map[rr_counter][curr_instruction->dst_index]= 
					map[rr_counter][curr_instruction->src1_index] - 
					curr_instruction->src2_index_imm ;
				thread_current_instruction[rr_counter]++;
				SIM_MemInstRead(thread_current_instruction[rr_counter],curr_instruction,rr_counter);
            	break;
        	case CMD_ADD:
				map[rr_counter][curr_instruction->dst_index]=
					 (map[rr_counter][curr_instruction->src1_index] + 
					 map[rr_counter][curr_instruction->src2_index_imm]) ;
				thread_current_instruction[rr_counter]++;
				SIM_MemInstRead(thread_current_instruction[rr_counter],curr_instruction,rr_counter);
				break;
        	case CMD_SUB:
            	map[rr_counter][curr_instruction->dst_index]=
					(map[rr_counter][curr_instruction->src1_index] - 
					map[rr_counter][curr_instruction->src2_index_imm]) ; 
				thread_current_instruction[rr_counter]++;
				SIM_MemInstRead(thread_current_instruction[rr_counter],curr_instruction,rr_counter);
            	break;
			
			}
			curr_cycle++;
		}
		if(curr_instruction->opcode == CMD_LOAD)
		{
			instructions_num_blocked++;
			map[rr_counter][9] = curr_cycle+SIM_GetLoadLat()+1;
			if(curr_instruction->isSrc2Imm)
			{
				SIM_MemDataRead(map[rr_counter][curr_instruction->src1_index] + curr_instruction->src2_index_imm,  &(map[rr_counter][curr_instruction->dst_index]));
			}
			else
			{
				SIM_MemDataRead(map[rr_counter][curr_instruction->src1_index] + map[rr_counter][curr_instruction->src2_index_imm],  &(map[rr_counter][curr_instruction->dst_index]));
			}
			thread_current_instruction[rr_counter]++;
			if(Next_RR(map,&rr_counter,curr_cycle,threads_num)!= -1)
				{
					curr_cycle+=SIM_GetSwitchCycles();
					
				}
		}
		else if(curr_instruction->opcode == CMD_STORE)
		{
			instructions_num_blocked++;
			map[rr_counter][9] = curr_cycle+SIM_GetStoreLat()+1;
			if(curr_instruction->isSrc2Imm)
			{
				SIM_MemDataWrite(map[rr_counter][curr_instruction->dst_index] + curr_instruction->src2_index_imm, map[rr_counter][curr_instruction->src1_index]);
			}
			else
			{
				SIM_MemDataWrite(map[rr_counter][curr_instruction->dst_index] + map[rr_counter][curr_instruction->src2_index_imm], map[rr_counter][curr_instruction->src1_index]);
			}
			thread_current_instruction[rr_counter]++;
			if(Next_RR(map,&rr_counter,curr_cycle,threads_num)!= -1)
				{
					curr_cycle+=SIM_GetSwitchCycles();
					
				}
		}
		else // HALT
		{
			instructions_num_blocked++;
			map[rr_counter][8] =0;
			active_threads--;
			if(Next_RR(map,&rr_counter,curr_cycle,threads_num)!= -1)
				{
					curr_cycle+=SIM_GetSwitchCycles();
					
				}
		}
		
		curr_cycle++;
		
	}
	blocked_cycles = curr_cycle;
	free(curr_instruction);
	free(thread_current_instruction);
	
}

void CORE_FinegrainedMT() {
	int threads_num = SIM_GetThreadsNum();
	uint32_t* thread_current_instruction = malloc(sizeof(int)*threads_num);
	for(int i=0; i<threads_num;i++)
	{	
		thread_current_instruction[i]=0;
	}
	int res=0;
	map = malloc(sizeof(int32_t*)*(threads_num));
	for(int i=0;i<threads_num;i++)
	{
		map[i] = malloc(sizeof(int32_t)*10); // 9 and 10 bit are to determine if the thread is active/inactive and in which cycle will i be ready
	}
	for(int i=0;i<threads_num;i++)
	{
		for(int j=0;j<8;j++)
		{
			map[i][j]=0;
		}
		map[i][8] = 1;
		map[i][9] = 0; 
	}
	int rr_counter =0; // the current thread running
	int active_threads = threads_num;
	int curr_cycle =0;
	Instruction* curr_instruction = malloc(sizeof(Instruction));
	while(active_threads>0)
	{	
		SIM_MemInstRead(thread_current_instruction[rr_counter],curr_instruction,rr_counter);
		instructions_num_fg++;
		switch (curr_instruction->opcode) 
		{
		case CMD_LOAD:
			
			map[rr_counter][9] = curr_cycle+SIM_GetLoadLat()+1;
			if(curr_instruction->isSrc2Imm)
			{
				SIM_MemDataRead(map[rr_counter][curr_instruction->src1_index] + curr_instruction->src2_index_imm,  &(map[rr_counter][curr_instruction->dst_index]));
			}
			else
			{
				SIM_MemDataRead(map[rr_counter][curr_instruction->src1_index] + map[rr_counter][curr_instruction->src2_index_imm],  &(map[rr_counter][curr_instruction->dst_index]));
			}
			thread_current_instruction[rr_counter]++;
			break;
		case CMD_STORE:
			
			map[rr_counter][9] = curr_cycle+SIM_GetStoreLat()+1;
			if(curr_instruction->isSrc2Imm)
			{
				SIM_MemDataWrite(map[rr_counter][curr_instruction->dst_index] + curr_instruction->src2_index_imm, map[rr_counter][curr_instruction->src1_index]);
			}
			else
			{
				SIM_MemDataWrite(map[rr_counter][curr_instruction->dst_index] + map[rr_counter][curr_instruction->src2_index_imm], map[rr_counter][curr_instruction->src1_index]);
			}
			thread_current_instruction[rr_counter]++;
			break;
		case CMD_HALT:
			map[rr_counter][8] =0;
			active_threads--;
			break;
		case CMD_NOP: // NOP
			break;
		case CMD_ADDI:
			map[rr_counter][curr_instruction->dst_index] = 
				map[rr_counter][curr_instruction->src1_index] + 
				curr_instruction->src2_index_imm;
			thread_current_instruction[rr_counter]++;
			break;
		case CMD_SUBI:
			map[rr_counter][curr_instruction->dst_index]= 
				map[rr_counter][curr_instruction->src1_index] - 
				curr_instruction->src2_index_imm ;
			thread_current_instruction[rr_counter]++;
			break;
		case CMD_ADD:
			map[rr_counter][curr_instruction->dst_index]=
					map[rr_counter][curr_instruction->src1_index] + 
					map[rr_counter][curr_instruction->src2_index_imm] ;
			thread_current_instruction[rr_counter]++;
			break;
		case CMD_SUB:
			map[rr_counter][curr_instruction->dst_index]=
				map[rr_counter][curr_instruction->src1_index] - 
				map[rr_counter][curr_instruction->src2_index_imm] ; 
			thread_current_instruction[rr_counter]++;
			break;
		
		}
	curr_cycle++;
	if(active_threads ==0) break;
	rr_counter = (rr_counter+1)%threads_num;
	res = Next_RR(map,&rr_counter,curr_cycle,threads_num);
	while(res == -1) //no avaliable threads
		{
			
			curr_cycle++;
			res = Next_RR(map,&rr_counter,curr_cycle,threads_num);
		}
	}
	fg_cycles = curr_cycle;
	free(thread_current_instruction);
	free(curr_instruction);
	
}

double CORE_BlockedMT_CPI(){
	int threads_num =SIM_GetThreadsNum();
	for(int i=0;i<threads_num;i++)
	{
		free(map[i]);
	}
	free(map);
	return ((double)blocked_cycles/(double)instructions_num_blocked);
}

double CORE_FinegrainedMT_CPI(){
	int threads_num =SIM_GetThreadsNum();
	for(int i=0;i<threads_num;i++)
	{
		free(map[i]);
	}
	free(map);
	return ((double)fg_cycles/(double)instructions_num_fg);
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for(int i =0;i<8;i++)
	{
		context[threadid].reg[i] = map[threadid][i];
	}
	
	
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for(int i =0;i<8;i++)
	{
		context[threadid].reg[i] = map[threadid][i];
	}
	
}
