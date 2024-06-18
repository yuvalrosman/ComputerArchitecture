/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <iomanip>


using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using std::vector;

#define EMPTYSPACE -1
#define DIRTY  -2
#define NOTDIRTY  -3






//-----------------------------------------------------------------------------------------


class CacheLevel{ //  a class to describe both layers of the cache as they share similar methods
public:
    unsigned size;
    unsigned associative_level;
    unsigned access_time;
    unsigned block_size;
    unsigned ways;
    unsigned sets;
    int*** addresses;
    unsigned hit_count;
    unsigned miss_count;
    CacheLevel() = default;
    CacheLevel(unsigned size, unsigned assoc_lev, unsigned time, unsigned b_size);
    ~CacheLevel();
    int findInCacheLevel(unsigned long address);
    int findSpace(unsigned address);
    void updateLRU(unsigned address, unsigned way);
    void insertAndUpdateLRU(unsigned address, unsigned way, int dirty);
    bool evictAndInsertL1(unsigned address, int* evict_address,int dirty);
    int evictAndInsertL2(unsigned address, int dirty);

};

CacheLevel::CacheLevel(unsigned int size, unsigned int assoc_lev, unsigned int time,unsigned b_size):size(size),associative_level(assoc_lev),access_time(time),block_size(b_size)
        ,hit_count(0),miss_count(0){
    ways = pow(2,assoc_lev);
    sets = (pow(2,size)/(pow(2,block_size)*ways));
    addresses = new int **[sets];
    for(int i=0;i<sets;i++)
    {
        addresses[i] = new int*[ways];
        for(int j=0;j<ways;j++)
        {
            addresses[i][j] = new int[4];
        }
    }
    for(int i=0;i<sets;i++)
    {

        for(int j=0;j<ways;j++)
        {
            addresses[i][j][0] = EMPTYSPACE; // tag
            addresses[i][j][1] = j; // lru count
            addresses[i][j][2] = NOTDIRTY; // dirty bit
            addresses[i][j][3] = EMPTYSPACE; //address
        }
    }
}
CacheLevel::~CacheLevel() {
    for (int i=0;i<sets;i++)
    {
        for(int j=0;j<ways;j++)
        {
            delete[] addresses[i][j];
        }
        delete[] addresses[i];
    }
    delete[] addresses;
}

int CacheLevel::findInCacheLevel(unsigned long address) { // the function recieves an address, calculates the set and way inside the cache and compares the tags, if found, returns the way
    unsigned long bits_of_sets_and_offset = (unsigned long)(address % (unsigned long)(pow(2,block_size+log2(sets))));
    unsigned long tag = (unsigned long)(address / (unsigned long)(pow(2,block_size+log2(sets))));
    unsigned long row_num = (unsigned long)(bits_of_sets_and_offset/(pow(2,block_size)));
    for(int i=0;i<ways;i++)
    {
        if((addresses)[row_num][i][0] == tag)
        {
            return i;
        }
    }
    return -1;
}

int CacheLevel::findSpace(unsigned int address) {// given an address, if the matching set has an open way, returns the way
    unsigned tag = unsigned(address / (unsigned )(pow(2,block_size+log2(sets))));
    unsigned bits_of_sets_and_offset = unsigned(address % (unsigned )(pow(2,block_size+log2(sets))));
    unsigned row_num = unsigned(bits_of_sets_and_offset/(pow(2,block_size)));
    for(int i=0;i<ways;i++)
    {
        if((addresses)[row_num][i][0] == EMPTYSPACE)
        {
            return i;
        }
    }
    return -1;
}

void CacheLevel::updateLRU(unsigned address, unsigned way) { // updates the lru field as learned in the lectures, given an address and a way that is the MRU
    unsigned bits_of_sets_and_offset = unsigned(address % (unsigned )(pow(2,block_size+log2(sets))));
    unsigned row_num = unsigned(bits_of_sets_and_offset/(pow(2,block_size)));
    unsigned x = (addresses)[row_num][way][1];
    (addresses)[row_num][way][1] = ways-1;
    for(int j=0;j<ways;j++)
    {
        if(j!=way && (addresses)[row_num][j][1]>x)
        {
            (addresses)[row_num][j][1]--;
        }
    }
}

void CacheLevel::insertAndUpdateLRU(unsigned address, unsigned way, int dirty) { // given an address, insert the new address and tag to the given way and update the sets LRU
    unsigned tag = unsigned(address / (unsigned )(pow(2,block_size+log2(sets))));
    unsigned bits_of_sets_and_offset = unsigned(address % (unsigned )(pow(2,block_size+log2(sets))));
    unsigned row_num = unsigned(bits_of_sets_and_offset/(pow(2,block_size)));
    (addresses)[row_num][way][0] = int(tag);
    (addresses)[row_num][way][2] = dirty;
    (addresses)[row_num][way][3] = address;
    updateLRU(address,way);

}
// if there is no place in L1 choose the LRU way and evict it. take the value of the address stored in evicted way and assign it to the pointer, returnes true if evicted was dirty, false otherwise
bool CacheLevel::evictAndInsertL1(unsigned int address, int* evict_address,int dirty) {
    bool res = true;
    int tag = int(address / (unsigned )(pow(2,block_size+log2(sets))));
    unsigned bits_of_sets_and_offset = unsigned(address % (unsigned )(pow(2,block_size+log2(sets))));
    unsigned row_num = unsigned(bits_of_sets_and_offset/(pow(2,block_size)));
    int evict_way=-1;
    for(int i =0;i<ways;i++)
    {
        if((addresses)[row_num][i][1] == 0)
        {
            evict_way =i;
            break;
        }
    }
    (addresses)[row_num][evict_way][0] = tag;
    *evict_address=(addresses)[row_num][evict_way][3];
    (addresses)[row_num][evict_way][3] = address;
    if((addresses)[row_num][evict_way][2] == DIRTY)
    {
        res = false;
    }
    (addresses)[row_num][evict_way][2] = dirty;
    updateLRU(address,evict_way);
    return res;
}

int CacheLevel::evictAndInsertL2(unsigned int address, int dirty) { //corresponding function for L2 cache
    int tag = int(address / (unsigned )(pow(2,block_size+log2(sets))));
    unsigned bits_of_sets_and_offset = unsigned(address % (unsigned )(pow(2,block_size+log2(sets))));
    unsigned row_num = unsigned(bits_of_sets_and_offset/(pow(2,block_size)));
    int evict_way=-1;
    for(int i =0;i<ways;i++)
    {
        if((addresses)[row_num][i][1] == 0)
        {
            evict_way =i;
            break;
        }
    }
    (addresses)[row_num][evict_way][0] = tag;
    (addresses)[row_num][evict_way][2] = dirty;
    int res =(addresses)[row_num][evict_way][3];
    (addresses)[row_num][evict_way][3] = address;
    updateLRU(address,evict_way);
    return res;
}




class Cache{ //Our DS, includes both L1 and L2 and keeps track of the access time and hit/miss count of each layer
public:
    unsigned mem_access_time;
    unsigned block_size;
    unsigned mode;
    CacheLevel* l1;
    CacheLevel* l2;
    int total_cycles;
    int total_commands;
    Cache();
    explicit Cache(unsigned cycles,unsigned block_size, unsigned mode, unsigned l1_size, unsigned l2_size,
                   unsigned l1_assoc_level, unsigned l2_assoc_level,
                   unsigned l1_access_time, unsigned l2_access_time);
    ~Cache();
    void Read(unsigned long int  address);
    void Write(unsigned long int address);
};

Cache::Cache(unsigned int cycles, unsigned int block_size, unsigned mode, unsigned int l1_size, unsigned int l2_size,
             unsigned int l1_assoc_level, unsigned int l2_assoc_level, unsigned int l1_access_time,
             unsigned int l2_access_time): mem_access_time(cycles), block_size(block_size),mode(mode),l1(new CacheLevel(l1_size,l1_assoc_level,l1_access_time,block_size)),
                                           l2(new CacheLevel(l2_size,l2_assoc_level,l2_access_time,block_size)),total_cycles(0),total_commands(0){}

Cache::~Cache() {
    delete l1;
    delete l2;
}

void Cache::Read(unsigned long int address)
{
    int l1findres =l1->findInCacheLevel(address);
    int l2findres =l2->findInCacheLevel(address);
    if(l1findres>=0) // data in l1 and l2
    {
        l1->updateLRU(address,l1findres);
        l1->hit_count++;
        total_cycles+=(l1->access_time);
    }
    else if(l2findres>=0) // data is in l2 and not! l1
    {
        l1->miss_count++;
        l2->updateLRU(address,l2findres);
        if(l1->findSpace(address)>=0) // there is space in l1
        {
            l1->insertAndUpdateLRU(address,l1->findSpace(address),NOTDIRTY);
        }
        else // there is no space and need to evict
        {
            int tmp_address;
            if(!l1->evictAndInsertL1(address, &tmp_address,NOTDIRTY))
            {
                //calculation of set and way
                unsigned tmp_bits_of_sets_and_offset = unsigned(tmp_address % (unsigned) (pow(2, block_size + log2(l2->sets))));
                unsigned tmp_row_num = unsigned(tmp_bits_of_sets_and_offset / (pow(2, block_size)));
                int tmp_way =l2->findInCacheLevel(tmp_address);
                (l2->addresses)[tmp_row_num][tmp_way][2] = DIRTY;
                l2->updateLRU(tmp_address,tmp_way);

            }
        }
        l2->hit_count++;
        total_cycles+=(l1->access_time+l2->access_time);
    }
    else // data is in mem
    {
        l1->miss_count++;
        l2->miss_count++;
        total_cycles+=(l1->access_time+l2->access_time+mem_access_time);
        if(l2->findSpace(address)>=0)
        {
            l2->insertAndUpdateLRU(address,l2->findSpace(address),NOTDIRTY);
        }
        else
        {
            int tmp2_address = l2->evictAndInsertL2(address, NOTDIRTY);
            int res = l1->findInCacheLevel(tmp2_address);
            if(res>=0)
            {
                //calculation of set and way
                unsigned new_bits_of_sets_and_offset2 = unsigned(tmp2_address % (unsigned) (pow(2, block_size + log2(l1->sets))));
                unsigned new_row_num2 = unsigned(new_bits_of_sets_and_offset2 / (pow(2, block_size)));
                ((l1->addresses))[new_row_num2][res][0] = EMPTYSPACE;
                ((l1->addresses))[new_row_num2][res][2] = NOTDIRTY;
                ((l1->addresses))[new_row_num2][res][3] = EMPTYSPACE;
            }
        }
        if(l1->findSpace(address)>=0)
        {
            l1->insertAndUpdateLRU(address,l1->findSpace(address),NOTDIRTY);
        }
        else
        {
            int tmp_address;
            if(!l1->evictAndInsertL1(address, &tmp_address,NOTDIRTY)) // if the condition is true, we need to update l2 to dirty
            {
                //calculation of set and way
                unsigned tmp_bits_of_sets_and_offset = unsigned(tmp_address % (unsigned) (pow(2, block_size + log2(l2->sets))));
                unsigned tmp_row_num = unsigned(tmp_bits_of_sets_and_offset / (pow(2, block_size)));
                int tmp_way =l2->findInCacheLevel(tmp_address);
                (l2->addresses)[tmp_row_num][tmp_way][2] = DIRTY;
                l2->updateLRU(tmp_address,tmp_way);
            }
        }
    }
}


void Cache::Write(unsigned long int address) {
    int l1findres =l1->findInCacheLevel(address);
    int l2findres =l2->findInCacheLevel(address);
    if(l1findres>=0) //  data is in l1
    {
        //calculation of set and way
        int tag = int(address / (unsigned )(pow(2,block_size+log2(l1->sets))));
        unsigned bits_of_sets_and_offset = unsigned(address % (unsigned )(pow(2,block_size+log2(l1->sets))));
        unsigned row_num = unsigned(bits_of_sets_and_offset/(pow(2,block_size)));
        l1->hit_count++;
        total_cycles+=(l1->access_time);
        ((l1->addresses))[row_num][l1findres][2] = DIRTY;
        l1->updateLRU(address,l1findres);
    }
    else if(l2findres>=0) // data is in l2
    {
        l1->miss_count++;
        //calculation of set and way
        int tag = int(address / (unsigned) (pow(2, block_size + log2(l2->sets))));
        unsigned bits_of_sets_and_offset = unsigned(address % (unsigned) (pow(2, block_size + log2(l2->sets))));
        unsigned row_num = unsigned(bits_of_sets_and_offset / (pow(2, block_size)));
        l2->hit_count++;
        total_cycles+=(l1->access_time+l2->access_time);
        ((l2->addresses))[row_num][l2findres][2] = NOTDIRTY;
        l2->updateLRU(address, l2findres);
        if (mode == 1) //WALLOC
        {
            if (l1->findSpace(address) >= 0) // there is space in l1
            {
                l1->insertAndUpdateLRU(address, l1->findSpace(address), DIRTY);
            }
            else // there is no space and need to evict
            {
                int tmp_address;
                if(!l1->evictAndInsertL1(address, &tmp_address,DIRTY))
                {
                    //calculation of set and way
                    unsigned new_bits_of_sets_and_offset = unsigned(tmp_address % (unsigned) (pow(2, block_size + log2(l2->sets))));
                    unsigned new_row_num = unsigned(new_bits_of_sets_and_offset / (pow(2, block_size)));
                    int l2_way_to_dirty = l2->findInCacheLevel(tmp_address);
                    ((l2->addresses))[new_row_num][l2_way_to_dirty][2] = DIRTY;
                    l2->updateLRU(tmp_address,l2_way_to_dirty);
                }
                //calculation of set and way
                unsigned bits_of_sets_and_offset_dirty = unsigned(address % (unsigned) (pow(2, block_size + log2(l1->sets))));
                unsigned row_num_dirty = unsigned(bits_of_sets_and_offset_dirty / (pow(2, block_size)));
                int way_to_dirty = l1->findInCacheLevel(address);
                (l1->addresses)[row_num_dirty][way_to_dirty][2] = DIRTY;
            }
        }
    }
    else
    { // data is only in mem
        l1->miss_count++;
        l2->miss_count++;
        total_cycles+=(l1->access_time+l2->access_time+mem_access_time);
        if (mode == 1) //WALLOC
        {
            if (l2->findSpace(address) >= 0)
            {
                l2->insertAndUpdateLRU(address, l2->findSpace(address), NOTDIRTY);
            }
            else
            {
                int tmp2_address=l2->evictAndInsertL2(address,NOTDIRTY);
                int l1tmpfind = l1->findInCacheLevel(tmp2_address);
                if(l1tmpfind>=0) {
                    //calculation of set and way
                    unsigned new_bits_of_sets_and_offset2 = unsigned(
                            tmp2_address % (unsigned) (pow(2, block_size + log2(l1->sets))));
                    unsigned new_row_num2 = unsigned(new_bits_of_sets_and_offset2 / (pow(2, block_size)));
                    ((l1->addresses))[new_row_num2][l1tmpfind][0] = EMPTYSPACE;
                    ((l1->addresses))[new_row_num2][l1tmpfind][3] = EMPTYSPACE;
                }
            }
            if (l1->findSpace(address) >= 0)
            {
                l1->insertAndUpdateLRU(address, l1->findSpace(address), DIRTY);
            }
            else // there is no space and need to evict
            {
                int tmp_address;
                if (!l1->evictAndInsertL1(address, &tmp_address,DIRTY)) {
                    //calculation of set and way
                    unsigned new_bits_of_sets_and_offset =unsigned(tmp_address % (unsigned) (pow(2, block_size + log2(l2->sets))));
                    unsigned new_row_num = unsigned(new_bits_of_sets_and_offset / (pow(2, block_size)));
                    int tmp_way = l2->findInCacheLevel(tmp_address);
                    ((l2->addresses))[new_row_num][tmp_way][2] = DIRTY;
                    l2->updateLRU(tmp_address,tmp_way);
                }
                //calculation of set and way
                unsigned bits_of_sets_and_offset = unsigned(address % (unsigned) (pow(2, block_size + log2(l1->sets))));
                unsigned row_num = unsigned(bits_of_sets_and_offset / (pow(2, block_size)));
                int way_to_dirty = l1->findInCacheLevel(address);
                (l1->addresses)[row_num][way_to_dirty][2] = DIRTY;
            }
        }
    }

    return;
}
//--------------------------------------------------------------------------








int main(int argc, char **argv) {

    if (argc < 19) {
        cerr << "Not enough arguments" << endl;
        return 0;
    }

    // Get input arguments

    // File
    // Assuming it is the first argument
    char* fileString = argv[1];
    ifstream file(fileString); //input file stream
    string line;
    if (!file || !file.good()) {
        // File doesn't exist or some other error
        cerr << "File not found" << endl;
        return 0;
    }

    unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
            L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

    for (int i = 2; i < 19; i += 2) {
        string s(argv[i]);
        if (s == "--mem-cyc") {
            MemCyc = atoi(argv[i + 1]);
        } else if (s == "--bsize") {
            BSize = atoi(argv[i + 1]);
        } else if (s == "--l1-size") {
            L1Size = atoi(argv[i + 1]);
        } else if (s == "--l2-size") {
            L2Size = atoi(argv[i + 1]);
        } else if (s == "--l1-cyc") {
            L1Cyc = atoi(argv[i + 1]);
        } else if (s == "--l2-cyc") {
            L2Cyc = atoi(argv[i + 1]);
        } else if (s == "--l1-assoc") {
            L1Assoc = atoi(argv[i + 1]);
        } else if (s == "--l2-assoc") {
            L2Assoc = atoi(argv[i + 1]);
        } else if (s == "--wr-alloc") {
            WrAlloc = atoi(argv[i + 1]);
        } else {
            cerr << "Error in arguments" << endl;
            return 0;
        }
    }

    //TODO:
    Cache* cache = new Cache(MemCyc,BSize,WrAlloc,L1Size,L2Size,L1Assoc,L2Assoc,L1Cyc,L2Cyc);

    while (getline(file, line)) {

        stringstream ss(line);
        string address;
        char operation = 0; // read (R) or write (W)
        if (!(ss >> operation >> address)) {
            // Operation appears in an Invalid format
            cout << "Command Format error" << endl;
            return 0;
        }

        // DEBUG - remove this line
        //cout << "operation: " << operation;

        string cutAddress = address.substr(2); // Removing the "0x" part of the address

        // DEBUG - remove this line
        //cout << ", address (hex)" << cutAddress;

        unsigned long int num = 0;
        num = strtoul(cutAddress.c_str(), NULL, 16);

        // DEBUG - remove this line
        //cout << " (dec) " << num << endl;


        if(operation == 'r') // read
        {
            cache->Read(num);
            cache->total_commands++;
        }
        else // write
        {
            cache->Write(num);
            cache->total_commands++;
        }

    }

    double L1MissRate = double((cache->l1->miss_count))/double(((cache->l1->miss_count)+(cache->l1->hit_count)));
    double L2MissRate = double((cache->l2->miss_count))/double(((cache->l2->miss_count)+(cache->l2->hit_count)));
    double avgAccTime = double(cache->total_cycles)/double(cache->total_commands);
    cerr<<std::fixed;
    cerr<<std::setprecision(3);
    //cerr<< "l1 miss is : " << L1MissRate << " l2 miss is :" << L2MissRate << " avg time is: " << avgAccTime << endl;
    printf("L1miss=%.03f ", L1MissRate);
    printf("L2miss=%.03f ", L2MissRate);
    printf("AccTimeAvg=%.03f\n", avgAccTime);

    return 0;
}