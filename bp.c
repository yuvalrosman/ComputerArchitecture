/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include "math.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#define ST 3
#define WT 2
#define WNT 1
#define SNT 0

#define Taken true
#define NotTaken false
#define MAXUNSIGNED 4294967295


//global variables - the predictor itself
unsigned** history_array_local;
unsigned global_history;
unsigned** fsm_array_local;
unsigned* fsm_array_global;
unsigned** btb_array;
unsigned type_of_share;
unsigned btb_size;
unsigned tag_size;
unsigned history_size;
bool local_hist;
unsigned default_fsm_state;
bool local_fsm;
unsigned branch_num;
unsigned flush_num;
unsigned size;
int what_share;
unsigned index_to_update_if_we_have_share;


//helper function for moving from int to binary
void int_to_bin_array(unsigned num, unsigned* arr)
{
    for(int i=31;i>=0;i--)
    {
        arr[i]=num%2;
        num=num/2;
    }
}
//helper function for moving from binary to int
unsigned bin_array_to_int(unsigned* arr,unsigned start,unsigned stop)
{
    unsigned num=0;
    for(int i=stop;i>=start;i--)
    {
        num+= arr[31-i]*(pow(2,i-start)); //problem??
    }
    return num;
}

//helper function for updating the fsm state
void update_predictor_state(unsigned* curr_state,bool taken_not_taken);
void update_predictor_state(unsigned* curr_state,bool taken_not_taken) //updates the predictor state based on whether the branch occurred or not (not based on  the prediction) - 4 states FSM
{
    if(*curr_state == ST)
    {
        if(taken_not_taken == Taken)
        {

        }
        else
        {
            *curr_state= WT;

        }
    }
    else if(*curr_state == WT)
    {
        if(taken_not_taken == Taken)
        {

            *curr_state = ST;

        }
        else
        {

            *curr_state= WNT;
        }

    }
    else if(*curr_state == WNT)
    {
        if(taken_not_taken == Taken)
        {

            *curr_state = WT;

        }
        else
        {

            *curr_state= SNT;
        }

    }
    else if(*curr_state == SNT)
    {

        if(taken_not_taken == Taken)
        {

            *curr_state = WNT;
        }

    }
}

//main functions
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared){
    btb_array = malloc(sizeof(unsigned*)*(btbSize));
    if(btb_array == NULL) //if malloc failed
    {
        return -1;
    }
    for(int i=0;i<btbSize;i++)
    {
        btb_array[i]= malloc(sizeof(unsigned)*3);
        if(btb_array[i]==NULL)
        {
            for(int j=0;j<i;j++)
            {
                free(btb_array[j]);
            }
            free(btb_array);
            return -1;
        }
        btb_array[i][0] = MAXUNSIGNED; // used max unsigned so we won't get tag override when we dont want it to happen (defined as max possible unsigned)
        btb_array[i][1] = 0;
        btb_array[i][2] = 0;
    }

    //dividing to 4 cases, and initializing according to the locality\globality of the history\fsm
    if(isGlobalTable)
    {
        if(isGlobalHist) //GTGH
        {
            fsm_array_global =malloc(sizeof(unsigned)*(pow(2,historySize)));
            if(fsm_array_global == NULL) //if malloc failed
            {
                for(int i=0;i<btbSize;i++)
                {
                    free(btb_array[i]);
                }
                free(btb_array);
                return -1;
            }
            for(int i=0;i<pow(2,historySize);i++)
            {
                fsm_array_global[i] = fsmState;
            }


        }
        else //GTLH
        {
            history_array_local = malloc(sizeof(unsigned*)*(btbSize));
            if(history_array_local == NULL)//if malloc failed
            {
                for(int i=0;i<btbSize;i++)
                {
                    free(btb_array[i]);
                }
                free(btb_array);
                return -1;
            }
            for(int i=0;i<btbSize;i++)
            {
                history_array_local[i] = malloc(sizeof(unsigned)*2 );
                if(history_array_local[i]==NULL)//if malloc failed
                {
                    for(int j=0;j<i;j++)
                    {
                        free(history_array_local[j]);
                    }
                    free(history_array_local);
                    for(int k=0;k<btbSize;k++)
                    {
                        free(btb_array[k]);
                    }
                    free(btb_array);
                    return -1;
                }
            }
            fsm_array_global =malloc(sizeof(unsigned)*(pow(2,historySize)));
            if(fsm_array_global == NULL)//if malloc failed
            {
                for(int i=0;i<btbSize;i++)
                {
                    free(btb_array[i]);
                    free(history_array_local[i]);
                }
                free(history_array_local);
                free(btb_array);
                return -1;
            }
            for(int i=0;i<pow(2,historySize);i++)
            {
                fsm_array_global[i] = fsmState; //sets the local fsm to the default value
            }
            for(int i=0;i<btbSize;i++)
            {
                for(int j=0;j<2;j++)
                {
                    history_array_local[i][j] = 0; // sets history to default value
                }
            }

        }
    }
    else
    {
        if(isGlobalHist) //LTGH
        {
            fsm_array_local =malloc(sizeof(unsigned*)*btbSize);
            if(fsm_array_local == NULL)//if malloc failed
            {
                for(int i=0;i<btbSize;i++)
                {
                    free(btb_array[i]);
                }
                free(btb_array);
                return -1;
            }
            for(int i=0;i<btbSize;i++)
            {
                fsm_array_local[i] = malloc(sizeof(unsigned)* (pow(2,historySize)));
                if(fsm_array_local[i] == NULL)//if malloc failed
                {
                    for(int j=0;j<i;j++)
                    {
                        free(fsm_array_local[j]);
                    }
                    free(fsm_array_local);
                    for(int k=0;k<btbSize;k++)
                    {
                        free(btb_array[k]);
                    }
                    free(btb_array);
                    return -1;

                }
                for(int j=0;j<pow(2,historySize);j++)
                {
                    fsm_array_local[i][j] = fsmState; // sets fsm to default value
                }
            }


        }
        else //LTLH
        {
            history_array_local = malloc(sizeof(unsigned*)*btbSize);
            if(history_array_local == NULL)//if malloc failed
            {
                for(int i=0;i<btbSize;i++)
                {
                    free(btb_array[i]);
                }
                free(btb_array);
                return -1;
            }
            fsm_array_local =malloc(sizeof(unsigned*)*btbSize);
            if(fsm_array_local == NULL)//if malloc failed
            {
                for(int i=0;i<btbSize;i++)
                {
                    free(btb_array[i]);
                }
                free(btb_array);
                free(history_array_local);
                return -1;
            }
            for(int i=0;i<btbSize;i++)
            {
                history_array_local[i] = malloc(sizeof(unsigned)* 2);
                if(history_array_local[i] == NULL)//if malloc failed
                {
                    for (int j = 0; j < i; j++)
                    {
                        free(history_array_local[j]);
                    }
                    free(history_array_local);
                    free(fsm_array_local);
                    for(int k=0;k<btbSize;k++)
                    {
                        free(btb_array[k]);
                    }
                    free(btb_array);
                    return -1;
                }
                fsm_array_local[i] = malloc(sizeof (unsigned)*(pow(2,historySize)));
                if(fsm_array_local[i]==NULL)//if malloc failed
                {
                    for (int j = 0; j < i; j++)
                    {
                        free(fsm_array_local[j]);
                    }
                    for (int m = 0; m <= i; m++)
                    {
                        free(history_array_local[m]);
                    }
                    free(history_array_local);
                    free(fsm_array_local);
                    for(int k=0;k<btbSize;k++)
                    {
                        free(btb_array[k]);
                    }
                    free(btb_array);
                    return -1;
                }
                for(int j=0;j<2;j++)
                {
                    history_array_local[i][j] = 0;//sets history to default value
                }
            }
            for(int i=0;i<btbSize;i++)
            {
                for(int j=0;j<pow(2,historySize);j++)
                {
                    fsm_array_local[i][j] = fsmState;//sets fsm to default value
                }
            }
        }
    }
    //initializing the global parameters of the predictor so we could use them in the future
    local_hist= !isGlobalHist;
    history_size= historySize;
    tag_size = tagSize;
    btb_size =btbSize;
    type_of_share = Shared;
    default_fsm_state = fsmState;
    local_fsm = !isGlobalTable;
    global_history = 0;
    what_share=Shared;
    return 1;
}


bool BP_predict(uint32_t pc, uint32_t *dst){
    unsigned pc_to_bin_arr[32];
    int_to_bin_array(pc,pc_to_bin_arr);
    unsigned btb_index = bin_array_to_int(pc_to_bin_arr,2,log2(btb_size)+1);
    unsigned curr_tag = bin_array_to_int(pc_to_bin_arr,log2(btb_size)+2,log2(btb_size)+1+tag_size);

    if(curr_tag!=btb_array[btb_index][0]) // meaning the branch is new to the btb and we need to predict NT and add it to the btb
    {

        if(what_share>0) //using share
        {
            if(what_share>1)//usinng share mid
            {
                if(local_hist)
                {
                    //calculating the right index of the fsm using share mid
                    unsigned pc_top_16_bits=pc;
                    pc_top_16_bits=pc_top_16_bits>>16;
                    pc_top_16_bits = pc_top_16_bits % (unsigned)(pow(2,history_size));
                    index_to_update_if_we_have_share = pc_top_16_bits^0;
                }
                else
                {
                    unsigned pc_top_16_bits=pc;
                    pc_top_16_bits= pc_top_16_bits/(pow(2,16));
                    pc_top_16_bits = pc_top_16_bits% ((unsigned)(pow(2,history_size)));
                    index_to_update_if_we_have_share = pc_top_16_bits^global_history;
                }
            }
            else //using share lsb
            {
                if(local_hist)
                {
                    //calculating the right index of the fsm using share lsb
                    unsigned pc_bottom_history_bits=pc/4;
                    pc_bottom_history_bits = pc_bottom_history_bits% (int)(pow(2,history_size));
                    index_to_update_if_we_have_share = pc_bottom_history_bits^0;
                }
                else
                {
                    unsigned pc_bottom_history_bits=pc/4;
                    pc_bottom_history_bits = pc_bottom_history_bits%(int)(pow(2,history_size));
                    index_to_update_if_we_have_share = pc_bottom_history_bits^global_history;

                }
            }
        }

        *dst = pc+4;
        return false;


    }
    //we will now predict according to the right fsm (divided to 8 situations - GHLT,LHLT ,GHGTmidshare,GHGTlsbshare,GHGnoshare,LHGTmidshare,LHGTlsbshare,LHGnoshare)
    if (local_fsm)
    {
        if (local_hist)  //LHLT
        {


            if(fsm_array_local[btb_index][history_array_local[btb_index][1]]>1)
            {
                *dst = btb_array[btb_index][1];
                return true;
            }
            else
            {
                *dst = pc+4;
                return false;
            }
        }
        else //GHLT
        {

            if(fsm_array_local[btb_index][global_history]>1)
            {
                *dst = btb_array[btb_index][1];
                return true;
            }
            else
            {
                *dst = pc+4;
                return false;
            }
        }
    }
    else
    {
        if(local_hist) //LHGT
        {

            if(what_share >0) //using Lshare
            {
                if(what_share>1)// using share_mid
                {
                    unsigned pc_top_16_bits=pc;
                    pc_top_16_bits=pc_top_16_bits>>16;
                    pc_top_16_bits = pc_top_16_bits % (unsigned)(pow(2,history_size));
                    unsigned index_of_right_fsm = pc_top_16_bits^history_array_local[btb_index][1];
                    index_to_update_if_we_have_share = index_of_right_fsm;
                    if (fsm_array_global[index_of_right_fsm] > 1) {
                        *dst = btb_array[btb_index][1];
                        return true;
                    } else {
                        *dst = pc + 4;
                        return false;
                    }

                }
                else // using share_lsb
                {
                    unsigned pc_bottom_history_bits=pc/4;
                    pc_bottom_history_bits = pc_bottom_history_bits% (unsigned)(pow(2,history_size));
                    unsigned index_of_right_fsm = pc_bottom_history_bits^history_array_local[btb_index][1];
                    index_to_update_if_we_have_share = index_of_right_fsm;
                    if (fsm_array_global[index_of_right_fsm] > 1) {
                        *dst = btb_array[btb_index][1];
                        return true;
                    } else {
                        *dst = pc + 4;
                        return false;
                    }
                }
            }
            else // not using share
            {

                if (fsm_array_global[history_array_local[btb_index][1]] > 1) {

                    *dst = btb_array[btb_index][1];
                    return true;
                } else {
                    *dst = pc + 4;
                    return false;
                }
            }
        }
        else //GHGT
        {
            if(what_share >0) //using Gshare
            {
                if(what_share>1)// using share_mid
                {
                    unsigned pc_top_16_bits=pc;
                    pc_top_16_bits= pc_top_16_bits/(pow(2,16));
                    pc_top_16_bits = pc_top_16_bits% ((unsigned)(pow(2,history_size)));
                    unsigned index_of_right_fsm = pc_top_16_bits^global_history;
                    index_to_update_if_we_have_share = index_of_right_fsm;

                    if (fsm_array_global[index_of_right_fsm] > 1) {
                        *dst = btb_array[btb_index][1];
                        return true;
                    } else {
                        *dst = pc + 4;
                        return false;
                    }

                }
                else // using share_lsb
                {
                    unsigned pc_bottom_history_bits=pc/4;
                    pc_bottom_history_bits = pc_bottom_history_bits%(int)(pow(2,history_size));
                    unsigned index_of_right_fsm = pc_bottom_history_bits^global_history;
                    index_to_update_if_we_have_share = index_of_right_fsm;

                    if (fsm_array_global[index_of_right_fsm] > 1) {
                        *dst = btb_array[btb_index][1];
                        return true;
                    } else
                    {
                        *dst = pc + 4;
                        return false;
                    }
                }
            }
            else// not using share
            {

                if(fsm_array_global[global_history]>1)
                {
                    *dst = btb_array[btb_index][1];

                    return true;
                }
                else
                {
                    *dst = pc+4;

                    return false;
                }
            }


        }
    }

}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{

    unsigned pc_to_bin_arr[32];
    int_to_bin_array(pc,pc_to_bin_arr);

    unsigned btb_index = bin_array_to_int(pc_to_bin_arr,2,log2(btb_size)+1);

    unsigned curr_tag = bin_array_to_int(pc_to_bin_arr,log2(btb_size)+2,log2(btb_size)+1+tag_size);


    if(curr_tag==btb_array[btb_index][0])
    { // meaning the branch is known to the btb and need to update the right fsm and history

        btb_array[btb_index][1] = targetPc;
        btb_array[btb_index][2] = ((btb_array[btb_index][2])*2 + (unsigned) taken)%(unsigned )(pow(2,history_size));
        if (local_fsm)
        {
            if (local_hist) { //LHLT
                update_predictor_state(&fsm_array_local[btb_index][history_array_local[btb_index][1]],
                                       taken);
                history_array_local[btb_index][1] =((history_array_local[btb_index][1])*2 +
                                                    (unsigned) taken)%(unsigned )(pow(2,history_size));

            }
            else
            { //GHLT
                update_predictor_state(&fsm_array_local[btb_index][global_history],
                                       taken);

                global_history= (global_history*2 + (unsigned) taken)%(unsigned )(pow(2,history_size));
            }
        }
        else
        {

            if (local_hist) //LHGT
            {
                if(what_share==0)
                    update_predictor_state(&fsm_array_global[history_array_local[btb_index][1]],
                                           taken);
                else
                    update_predictor_state(&fsm_array_global[index_to_update_if_we_have_share],taken);


                history_array_local[btb_index][1] =((history_array_local[btb_index][1])*2 +
                                                    (unsigned) taken)%(unsigned )(pow(2,history_size));

            }
            else //GHGT
            {

                if(what_share==0)
                    update_predictor_state(&fsm_array_global[global_history],
                                           taken);
                else
                    update_predictor_state(&fsm_array_global[index_to_update_if_we_have_share],taken);
                int addition=0;
                if(taken) addition=1;
                global_history =((global_history*2) +
                                 addition)%((unsigned )(pow(2,history_size)));
            }
        }
    }
    else
    { //empty btb index or overriding existing branch
        btb_array[btb_index][1] = targetPc;
        btb_array[btb_index][0] = curr_tag;
        btb_array[btb_index][2] = (unsigned) taken;
        unsigned curr_fsm_state = default_fsm_state;
        update_predictor_state(&curr_fsm_state, taken);

        if (local_fsm)
        {
            if (local_hist) {//LHLT
                for (int i = 1; i < pow(2, history_size); i++) {
                    fsm_array_local[btb_index][i] = default_fsm_state;
                }
                fsm_array_local[btb_index][0] = curr_fsm_state;
                history_array_local[btb_index][0] = curr_tag;
                history_array_local[btb_index][1] = (unsigned) taken;
            }
            else
            { //
                for (int i = 0; i < pow(2, history_size); i++) {
                    fsm_array_local[btb_index][i] = default_fsm_state;
                }
                fsm_array_local[btb_index][global_history] = curr_fsm_state;

                global_history =
                        (global_history * 2 + (unsigned) taken) % (unsigned) (pow(2, history_size));
            }
        }
        else
        {
            if (local_hist)
            { //LHGT
                if(what_share==0)
                    update_predictor_state(&fsm_array_global[0], taken); // maybe update_predictor_state
                else update_predictor_state(&fsm_array_global[index_to_update_if_we_have_share], taken);

                history_array_local[btb_index][0] = curr_tag;
                history_array_local[btb_index][1] = (unsigned) taken;
            }
            else
            { //GHGT
                if(what_share==0)
                    update_predictor_state(&fsm_array_global[global_history], taken);
                else
                    update_predictor_state(&fsm_array_global[index_to_update_if_we_have_share],taken);

                int addition=0;
                if(taken) addition=1;
                global_history =((global_history * 2) + addition) % ((unsigned) (pow(2, history_size)));

            }

        }
    }
    if(((pred_dst != targetPc) && taken) || ((pred_dst != pc+4) && !taken))
    {
        flush_num++;
    }
    branch_num++;





}

void BP_GetStats(SIM_stats *curStats){

    //calculating the size of the whole predictor(for each of the 4 states) and freeing allocated memory
    curStats->br_num =branch_num;
    curStats->flush_num =flush_num;
    if (local_fsm)
    {
        if (local_hist)  //LHLT
        {
            curStats->size = btb_size*(tag_size+30+1)+btb_size*(history_size+(2*(pow(2,history_size))));
            for(int i =0;i<btb_size;i++)
            {
                free(fsm_array_local[i]);
                free(history_array_local[i]);
                free(btb_array[i]);
            }
            free(fsm_array_local);
            free(history_array_local);
            free(btb_array);
        }
        else //GHLT
        {
            curStats->size = btb_size*(tag_size+30+1)+btb_size*((2*(pow(2,history_size)))) +history_size;
            for(int i =0;i<btb_size;i++)
            {
                free(fsm_array_local[i]);
                free(btb_array[i]);
            }
            free(fsm_array_local);
            free(btb_array);
        }
    }
    else
    {
        if(local_hist) //LHGT
        {
            curStats->size = btb_size*(tag_size+30+1)+btb_size*(history_size)+(2*(pow(2,history_size)));
            for(int i =0;i<btb_size;i++)
            {
                free(history_array_local[i]);
                free(btb_array[i]);
            }
            free(fsm_array_global);
            free(history_array_local);
            free(btb_array);
        }
        else //GHGT
        {
            curStats->size = btb_size*(tag_size + 30+1) + history_size  + 2*(pow(2,history_size));
            for(int i =0;i<btb_size;i++)
            {
                free(btb_array[i]);
            }
            free(fsm_array_global);
            free(btb_array);
        }
    }

}