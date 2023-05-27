#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

struct address{
    int valid;
    unsigned long tag;
    int age;
    unsigned long data;
};

int main(int argc, char* argv[argc+1]){
    FILE* query = fopen(argv[8], "r");
    int L1cacheSize = atoi(argv[1]);
    int L1blockSize = atoi(argv[4]); 
    char total[1000];
    for(int i=0;i<sizeof(argv[2]);i++){
        total[i] = argv[2][6+i];
    }
    int L1blocksInSet = atoi(total);
    int L1setCount = (L1cacheSize/(L1blocksInSet * L1blockSize));

    int L2cacheSize = atoi(argv[5]);
    char total2[1000];
    for(int i=0;i<sizeof(argv[2]);i++){
        total2[i] = argv[6][6+i];
    }
    int L2blocksInSet = atoi(total2);
    int L2setCount = (L2cacheSize/(L2blocksInSet*L1blockSize)); 

    struct address** L1 = malloc(sizeof(struct address)*L1setCount);
    for(int i=0 ; i<L1setCount; i++){
        L1[i]=malloc(sizeof(struct address)*L1blocksInSet);
    }

    struct address** L2 = malloc(sizeof(struct address)*L2setCount);
    for(int i=0 ; i<L2setCount; i++){
        L2[i]=malloc(sizeof(struct address)*L2blocksInSet);
    }
    //L1 cache policy in argv[3]
    //L2 cache policy in argv[7]
    
    for(int i=0; i<L1setCount; i++){
        for(int j=0; j<L1blocksInSet; j++){
            L1[i][j].valid=0;
            L1[i][j].age=0;
        }
    }

    for(int i=0; i<L2setCount; i++){
        for(int j=0; j<L2blocksInSet; j++){
            L2[i][j].valid=0;
            L2[i][j].age=0;
        }
    }

    char operation;
    unsigned long address;
    int setCountNew = log(L1setCount)/log(2);
    int L2setCountNew = log(L2setCount)/log(2);
    int BlockCountNew = log(L1blockSize)/log(2);
    int L1cacheHits=0;
    int L1cacheMisses=0;
    int L2cacheHits=0;
    int L2cacheMisses=0;
    //int memread=0;
    int memwrite=0;
    //memread is always equal to L2cacheMisses

    while(fscanf(query, "%c %lx\n", &operation, &address)!=EOF){

        unsigned long L1tag = (address>>(setCountNew+BlockCountNew));
        unsigned long L1setIndex = (address>>BlockCountNew) & ((1<<setCountNew)-1);

        unsigned long L2tag = (address>>(L2setCountNew+BlockCountNew));
        unsigned long L2setIndex = (address>>BlockCountNew) & ((1<<L2setCountNew)-1);

        int L1HitUpdated=0;
        int L2HitUpdated = 0;

        for(int blockNumber=0; blockNumber<L1blocksInSet; blockNumber++){
            if(L1[L1setIndex][blockNumber].tag == L1tag){
                L1HitUpdated=1;
                L1cacheHits++;
                if(strcmp(argv[3],"lru")==0){
                    int greatestAge = L1[L1setIndex][0].age;
                    for(int i=0; i<L1blocksInSet; i++){
                        if(greatestAge<L1[L1setIndex][i].age){
                            greatestAge = L1[L1setIndex][i].age;
                        }
                    } 
                    L1[L1setIndex][blockNumber].age=greatestAge+1;
                }
                break;
            }
        }
        if(L1HitUpdated==0){
            L1cacheMisses++;
        }

        int L1blockfullCount=0;;
        for(int blockNumber=0; blockNumber<L1blocksInSet; blockNumber++){
            if(L1[L1setIndex][blockNumber].valid==1){
                L1blockfullCount++;
            }
        }

        if(operation =='R' || operation=='W'){
            //If it is a cache hit, already increment ONLY cacheHits++ once above

            if(operation=='W'){
                memwrite++;
            }
            
            //if L1 cacheMiss
            if(L1HitUpdated==0){
                //if L2 cache hit, then go down to else command
                int blockToRemove;
                for(int blockNumber=0; blockNumber<L2blocksInSet; blockNumber++){
                    if(L2[L2setIndex][blockNumber].valid==1 && L2[L2setIndex][blockNumber].tag == L2tag){
                        L2HitUpdated=1;
                        L2cacheHits++;
                        blockToRemove=blockNumber;
                        if(strcmp(argv[7],"lru")==0){
                            int greatestAge = L2[L2setIndex][0].age;
                            for(int i=0; i<L2blocksInSet; i++){
                                if(greatestAge<L2[L2setIndex][i].age){
                                    greatestAge = L2[L2setIndex][i].age;
                                }
                            } 
                            L2[L2setIndex][blockNumber].age=greatestAge+1;
                        }
                        break;
                    }
                }
                //If L2 cacheMiss as well
                if(L2HitUpdated==0){
                    L2cacheMisses++;
                    //if L1 is NOT FULL : L1 has some space
                    if(L1blockfullCount!=L1blocksInSet){
                        for(int blockNumber=0; blockNumber<L1blocksInSet; blockNumber++){
                            if(L1[L1setIndex][blockNumber].valid==0){
                                L1[L1setIndex][blockNumber].valid=1;
                                int greatestAge = L1[L1setIndex][0].age;
                                for(int i=0; i<L1blocksInSet; i++){
                                    if(greatestAge<L1[L1setIndex][i].age){
                                        greatestAge = L1[L1setIndex][i].age;
                                    }
                                } 
                                L1[L1setIndex][blockNumber].age=greatestAge+1;
                                L1[L1setIndex][blockNumber].tag=L1tag;
                                L1[L1setIndex][blockNumber].data = address;

                                break;
                            }
                        }             
                    }
                    //If L1 is full : place into L1 and take L1evicted and put into L2
                    else{

                        //getting block to evict from L1 since it is full
                        int blockMinAge=L1[L1setIndex][0].age;
                        int blockIndexMin=0;
                        for(int blockNumber=0; blockNumber<L1blocksInSet; blockNumber++){
                            if(blockMinAge>L1[L1setIndex][blockNumber].age){
                                blockMinAge = L1[L1setIndex][blockNumber].age;
                                blockIndexMin = blockNumber;
                            }
                        }
                        unsigned long tempAddress = L1[L1setIndex][blockIndexMin].data;
                        L1[L1setIndex][blockIndexMin].data = address;
                        L1[L1setIndex][blockIndexMin].valid=1;
                        L1[L1setIndex][blockIndexMin].tag = L1tag;
                        int greatestAge = L1[L1setIndex][0].age;
                        for(int i=0; i<L1blocksInSet; i++){
                            if(greatestAge<L1[L1setIndex][i].age){
                                greatestAge = L1[L1setIndex][i].age;
                            }
                        } 
                        L1[L1setIndex][blockIndexMin].age=greatestAge+1;
                        L2tag = (tempAddress>>(L2setCountNew+BlockCountNew));
                        L2setIndex = (tempAddress>>BlockCountNew) & ((1<<L2setCountNew)-1);
                        int L2blockfullCount=0;
                        for(int blockNumber=0; blockNumber<L2blocksInSet; blockNumber++){
                            if(L2[L2setIndex][blockNumber].valid==1){
                                L2blockfullCount++;
                            }
                        }
                        //if L2 is NOT full : take evicted L1 data and add to L2
                        if(L2blockfullCount!=L2blocksInSet){
                            for(int blockNumber=0; blockNumber<L2blocksInSet; blockNumber++){
                                if(L2[L2setIndex][blockNumber].valid==0){
                                    L2[L2setIndex][blockNumber].valid=1;
                                    int greatestAge = L2[L2setIndex][0].age;
                                    for(int i=0; i<L2blocksInSet; i++){
                                        if(greatestAge<L2[L2setIndex][i].age){
                                            greatestAge = L2[L2setIndex][i].age;
                                        }
                                    } 
                                    L2[L2setIndex][blockNumber].age=greatestAge+1;
                                    L2[L2setIndex][blockNumber].tag=L2tag;
                                    L2[L2setIndex][blockNumber].data = tempAddress;
                                    break;
                                }
                            }       
                        }
                        //if L2 is full: evict from L2 and just garbage L2 evicted
                        else{
                            int blockMinAge=L2[L2setIndex][0].age;
                            int blockIndexMin=0;
                            for(int blockNumber=0; blockNumber<L2blocksInSet; blockNumber++){
                                if(blockMinAge>L2[L2setIndex][blockNumber].age){
                                    blockMinAge = L2[L2setIndex][blockNumber].age;
                                    blockIndexMin = blockNumber;
                                }
                            }
                            L2[L2setIndex][blockIndexMin].valid=1;
                            L2[L2setIndex][blockIndexMin].tag = L2tag;
                            L2[L2setIndex][blockIndexMin].data = tempAddress;

                            int greatestAge = L2[L2setIndex][0].age;
                            for(int i=0; i<L2blocksInSet; i++){
                                if(greatestAge<L2[L2setIndex][i].age){
                                    greatestAge = L2[L2setIndex][i].age;
                                }
                            } 
                            L2[L2setIndex][blockIndexMin].age=greatestAge+1;
                        }

                    }

                }
                //if L2 cache hit : take block from L2 and move into L1
                else{
                    unsigned long tempAddress = L2[L2setIndex][blockToRemove].data;
                    L2[L2setIndex][blockToRemove].valid=0;
                    L2[L2setIndex][blockToRemove].age=0;
                    L1tag = (tempAddress>>(setCountNew+BlockCountNew));
                    L1setIndex = (tempAddress>>BlockCountNew) & ((1<<setCountNew)-1);
                    int newBlocksInCount=0;
                    for(int blockNumber=0; blockNumber<L1blocksInSet; blockNumber++){
                        if(L1[L1setIndex][blockNumber].valid==1){
                            newBlocksInCount++;
                        }
                    }
                    //case where L1 cache has space left : NOT full
                    if(newBlocksInCount!=L1blocksInSet){
                        for(int blockNumber=0; blockNumber<L1blocksInSet; blockNumber++){
                            if(L1[L1setIndex][blockNumber].valid==0){
                                L1[L1setIndex][blockNumber].valid=1;
                                int greatestAge = L1[L1setIndex][0].age;
                                for(int i=0; i<L1blocksInSet; i++){
                                    if(greatestAge<L1[L1setIndex][i].age){
                                        greatestAge = L1[L1setIndex][i].age;
                                    }
                                } 
                                L1[L1setIndex][blockNumber].age=greatestAge+1;
                                L1[L1setIndex][blockNumber].tag=L1tag;
                                L1[L1setIndex][blockNumber].data = tempAddress;

                                break;
                            }
                        }
                    }
                    //if L1 cache is full : then evict from L1 and place into L2 as normal
                    else{
                        //getting block to evict from L1 since it is full
                        int blockMinAge=L1[L1setIndex][0].age;
                        int blockIndexMin=0;
                        for(int blockNumber=0; blockNumber<L1blocksInSet; blockNumber++){
                            if(blockMinAge>L1[L1setIndex][blockNumber].age){
                                blockMinAge = L1[L1setIndex][blockNumber].age;
                                blockIndexMin = blockNumber;
                            }
                        }
                        unsigned long tempAddress2 = L1[L1setIndex][blockIndexMin].data;
                        L1[L1setIndex][blockIndexMin].data = tempAddress;
                        L1[L1setIndex][blockIndexMin].valid=1;
                        L1[L1setIndex][blockIndexMin].tag = L1tag;
                        int greatestAge = L1[L1setIndex][0].age;
                        for(int i=0; i<L1blocksInSet; i++){
                            if(greatestAge<L1[L1setIndex][i].age){
                                greatestAge = L1[L1setIndex][i].age;
                            }
                        } 
                        L1[L1setIndex][blockIndexMin].age=greatestAge+1;
                        L2tag = (tempAddress2>>(L2setCountNew+BlockCountNew));
                        L2setIndex = (tempAddress2>>BlockCountNew) & ((1<<L2setCountNew)-1);
                        int L2blockfullCount=0;
                        for(int blockNumber=0; blockNumber<L2blocksInSet; blockNumber++){
                            if(L2[L2setIndex][blockNumber].valid==1){
                                L2blockfullCount++;
                            }
                        }
                        //if L2 is NOT full : take evicted L1 data and add to L2
                        if(L2blockfullCount!=L2blocksInSet){
                            for(int blockNumber=0; blockNumber<L2blocksInSet; blockNumber++){
                                if(L2[L2setIndex][blockNumber].valid==0){
                                    L2[L2setIndex][blockNumber].valid=1;
                                    int greatestAge = L2[L2setIndex][0].age;
                                    for(int i=0; i<L2blocksInSet; i++){
                                        if(greatestAge<L2[L2setIndex][i].age){
                                            greatestAge = L2[L2setIndex][i].age;
                                        }
                                    } 
                                    L2[L2setIndex][blockNumber].age=greatestAge+1;
                                    L2[L2setIndex][blockNumber].tag=L2tag;
                                    L2[L2setIndex][blockNumber].data = tempAddress2;
                                    break;
                                }
                            }       
                        }
                        //if L2 is full: evict from L2 and just garbage L2 evicted
                        else{
                            int blockMinAge=L2[L2setIndex][0].age;
                            int blockIndexMin=0;
                            for(int blockNumber=0; blockNumber<L2blocksInSet; blockNumber++){
                                if(blockMinAge>L2[L2setIndex][blockNumber].age){
                                    blockMinAge = L2[L2setIndex][blockNumber].age;
                                    blockIndexMin = blockNumber;
                                }
                            }
                            L2[L2setIndex][blockIndexMin].valid=1;
                            L2[L2setIndex][blockIndexMin].tag = L2tag;
                            L2[L2setIndex][blockIndexMin].data = tempAddress2;

                            int greatestAge = L2[L2setIndex][0].age;
                            for(int i=0; i<L2blocksInSet; i++){
                                if(greatestAge<L2[L2setIndex][i].age){
                                    greatestAge = L2[L2setIndex][i].age;
                                }
                            } 
                            L2[L2setIndex][blockIndexMin].age=greatestAge+1;
                        }


                    }             



                }
            }

        }
    }
    printf("memread:%d\n",L2cacheMisses);
    printf("memwrite:%d\n",memwrite);
    printf("l1cachehit:%d\n",L1cacheHits);
    printf("l1cachemiss:%d\n",L1cacheMisses);
    printf("l2cachehit:%d\n",L2cacheHits);
    printf("l2cachemiss:%d\n",L2cacheMisses);

    for(int i=0; i<L1setCount; i++){
        free(L1[i]);
    }
    free(L1);

    for(int i=0; i<L2setCount; i++){
        free(L2[i]);
    }
    free(L2);
}