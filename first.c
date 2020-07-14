#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct Set{
	int time;
	int vbit;
	unsigned long long int tag;
} set;

typedef struct Cache{
	set * block;
} cache;

cache * scache;
int hits = 0;
int misses = 0;
int memreads = 0;
int memwrites  = 0;
int count = 0;
int prefetch = 0;
int numsets = 0;
int numblocks = 0;
unsigned long long int indexbits;
unsigned long long int offsetbits;

void associativitySetup(char *, int, int);
void setupCache();
unsigned long long int findTag(int, unsigned long);
void checkOperation(int, char *);
void DirectMapping(char *, unsigned long long int, unsigned long, unsigned long long int, int);
void fifo(char *, unsigned long long int, unsigned long, int, unsigned long long int);
void prefetching(char *, unsigned long long int, unsigned long, int, unsigned long long int);

void associativitySetup(char * associativity, int csize, int bsize){
      char * size;
		char * delim = ":";
		
	if(strcmp(associativity, "direct") == 0){
		numsets = csize/bsize;
		numblocks = 1;
	}else if(strcmp(associativity, "assoc") == 0){
		numsets = 1;
		numblocks = csize/bsize;
	}else if(associativity[0]=='a' && associativity[1]=='s'&& associativity[2]=='s'&& associativity[3]=='o'&& associativity[4]=='c'&& associativity[5]==':'){
		size = strtok(associativity, delim);
	   size = strtok(NULL, delim);
		numblocks = (int) strtol(size, NULL, 10);
		numsets = csize/(bsize * numblocks);

	}else{
		printf("error\n");
		exit(0);
	}
}
	
void setupCache(){
	int i;
	int j;
	
	scache = (cache*) malloc(sizeof(cache) * numsets); 
	
	for(i = 0; i < numsets; i++){
		for(j = 0; j < numblocks; j++){
			scache[i].block = (set*) malloc(sizeof(set) * numblocks);
			scache[i].block[j].vbit = 0;
			scache[i].block[j].time = 0;
			scache[i].block[j].tag = 0;
		}
	}
	
}

unsigned long long int findTag(int bsize, unsigned long address){
	unsigned long long int tag;


	indexbits = log2(numsets);	
	offsetbits = log2(bsize);      
	
	tag = (unsigned long long int)address >> (indexbits + offsetbits); 
	
	return tag;
	
}

void checkOperation(int check, char * op){
	
	if(check == 1){
		if(strcmp(op, "W") == 0){
			misses++;
			memreads++;
			memwrites++;
			count++;
			return;
		}else{
			misses++;
			memreads++;
			count++;
			return;
		}
	}else if(check == 2){
		if(strcmp(op, "W") == 0){
	   		hits++;
	   		memwrites++;
	   		count++;
	   		return;
	   	}else{
	   		hits++;
	   		count++;
	   		return;
	   	}
	}if(check == 3){
		if(strcmp(op, "W") == 0){
		   misses++;
			memreads++;
			memwrites++;
			count++;
			return;
		}else{
			misses++;
			memreads++;
	   	count++;
	   	return;
		}
		
		
		
	}
	return;
}

void DirectMapping(char * op, unsigned long long int curtag, unsigned long index, unsigned long long int address, int bsize){
	unsigned long long int preaddress;
	unsigned long long int pretag;
	unsigned long preindex;
	int check = 0; 

	
	if(prefetch == 1){
		index = (address >> offsetbits) & ((1 << indexbits)-1);
	   preaddress = address + bsize;
	   pretag = findTag(bsize, preaddress);
	   preindex = (preaddress >> offsetbits) & ((1 << indexbits)-1);	
	}

		//case 1: cold miss
		
	if(scache[index].block[0].vbit == 0){
		scache[index].block[0].vbit = 1;
		scache[index].block[0].tag = curtag;
		
		check = 1;
		
		checkOperation(check, op);
		
		if(prefetch == 1){
			if(scache[preindex].block[0].tag != pretag){
				scache[preindex].block[0].vbit = 1;
		      scache[preindex].block[0].tag = pretag;
		      memreads++;
		   }
		}
		
		return;
					
	   //case 2: hit	
					
	}else if(scache[index].block[0].tag == curtag && scache[index].block[0].vbit == 1){
		
		check = 2;
		checkOperation(check, op);
		return;
		
	//case 3: override
	}else{
		scache[index].block[0].tag = curtag;
		
		if(prefetch == 1){
			if(scache[preindex].block[0].tag != pretag){
				scache[preindex].block[0].vbit = 1;
		      scache[preindex].block[0].tag = pretag;
		      memreads++;
		   }
		}
		check = 3;
		
		checkOperation(check,op);
		return;
		
	}
	return;
	
}

void fifo(char * op, unsigned long long int curtag, unsigned long index, int bsize, unsigned long long int address){
	int toreplace = 0;
	int i;
	int check = 0;
  
	for(i = 0; i < numblocks; i++){
		//case 1: cold miss
		if(scache[index].block[i].vbit == 0){
			scache[index].block[i].vbit = 1;
			scache[index].block[i].tag = curtag;
			scache[index].block[i].time = count;
			
			check = 1;
			
			checkOperation(check, op);
		
			if(prefetch == 1){
				prefetching(op, curtag, index, bsize, address);
			}
			
			return;
		
		//case 2: hit	
	   }else if(scache[index].block[i].tag == curtag && scache[index].block[i].vbit == 1 ){
	   	check = 2;
	   	checkOperation(check, op);
	   	return;
	   		
	   //case 3: fifo		
	   }else{
	   	if(scache[index].block[toreplace].time > scache[index].block[i].time){
	   		toreplace = i;
	   	}
	   }
	}
	check = 3;

	scache[index].block[toreplace].vbit = 1;
	scache[index].block[toreplace].tag = curtag;
   scache[index].block[toreplace].time	= count;
   
   checkOperation(check, op);
   
   if(prefetch == 1){

				prefetching(op, curtag, index, bsize, address);
			}
	 
	return;
}

void prefetching(char * op, unsigned long long int curtag, unsigned long index, int bsize, unsigned long long int address){
	unsigned long long int pretag;
	unsigned long preindex; 
	unsigned long preaddress;
	int i;
	int preplace = 0;
	
		index = (address >> offsetbits) & ((1 << indexbits)-1);
	   preaddress = address + bsize;
	   pretag = findTag(bsize, preaddress);
	   preindex = (preaddress >> offsetbits) & ((1 << indexbits)-1); 
	 
	for(i = 0; i < numblocks; i++){
		if(scache[preindex].block[i].vbit == 0){
			scache[preindex].block[i].vbit = 1;
			scache[preindex].block[i].tag = pretag;
			scache[preindex].block[i].time = count; 
			count++;
			memreads++;
			return;
		}else if(scache[preindex].block[i].tag == pretag){
			return;
		}else if(scache[preindex].block[preplace].time > scache[preindex].block[i].time){
			preplace = i;
		}
	}
	
	scache[preindex].block[preplace]. tag = pretag;
	scache[preindex].block[preplace]. time = count;
	count++;
	memreads++;
	return;
	 
}
	

int main(int argc, char** argv){
	FILE * fptr;
	int csize = atoi(argv[1]);
	char * associativity = argv[2];
	char * policy = argv[3];
	int bsize = atoi(argv[4]);
	char pc[100];
   char op[2];
	unsigned long address = 0;
	unsigned long long int curtag = 0;
	unsigned long index = 0;

	//checks number of inputs
	if(argc != 6){ 
	   printf("error\n");
    	exit(0);
	}
	
	//checks if cache size is a power of 2
	if(csize % 2 != 0 && csize != 1){
		printf("error\n");
		exit(0);
	}
	
	//checks replacement poli1cy
	if(strcmp(policy, "fifo") != 0){
		printf("error\n");
		exit(0);
	}
	
	//checks block size
	if(bsize % 2 != 0 && bsize != 1){
	printf("error\n");
	exit(0);
}
	
   associativitySetup(associativity, csize, bsize);
		//printf("hi\n");
	//initialize cache
	setupCache();
	
	fptr = fopen(argv[5], "r");

	if(fptr == NULL){
		printf("error\n");
		exit(0);
	}
	
	indexbits = log2(numsets);	
	offsetbits = log2(bsize);
	
	while (fgetc(fptr) != '#'){

		fscanf(fptr,"%s\n %s\n %lx\n", pc, op, &address);

		index = (address >> offsetbits) & ((1 << indexbits)-1);
		curtag = findTag(bsize,address);
		
		if(strcmp(associativity, "direct") == 0){
			
			DirectMapping(op, curtag, index, address, bsize);
			
		}else{
			
			fifo(op, curtag, index, bsize, address);
			
		}
	}
	fclose(fptr);
	
	printf("no-prefetch\n");
	printf("Memory reads: %d\n",memreads);
   printf("Memory writes: %d\n",memwrites);
	printf("Cache hits: %d\n",hits);
	printf("Cache misses: %d\n",misses);
	
	prefetch = 1;
	memreads = 0;
	memwrites = 0;
	hits = 0;
	misses = 0;
	int i;
	int j;
	
	for(i = 0; i < numsets; i++){
		for(j = 0; j < numblocks; j++){
			scache[i].block[j].vbit = 0;
			scache[i].block[j].time = 0;
			scache[i].block[j].tag = 0;
		}
	}
	
	fptr = fopen(argv[5], "r");
	
	while (fgetc(fptr) != '#'){

		fscanf(fptr,"%s\n %s\n %lx\n", pc, op, &address);
		index = (address >> offsetbits) & ((1 << indexbits)-1);
		curtag = findTag(bsize, address);
		if(strcmp(associativity, "direct") == 0){
			DirectMapping(op, curtag, index, address, bsize);
			}else{
			fifo(op, curtag, index, bsize, address);
			}
	}
	fclose(fptr);
	
	printf("with-prefetch\n");
	printf("Memory reads: %d\n",memreads);
   printf("Memory writes: %d\n",memwrites);
	printf("Cache hits: %d\n",hits);
	printf("Cache misses: %d\n",misses);
	
	for(i = 0; i < numsets; i++){
			free(scache[i].block);
		
	}
	
	free(scache);
		

	return 0;
		
}