// 주의사항
// 1. hybridmapping.h에 정의되어 있는 상수 변수를 우선적으로 사용해야 함 
// (예를 들면, PAGES_PER_BLOCK의 상수값은 채점 시에 변경할 수 있으므로 반드시 이 상수 변수를 사용해야 함)
// 2. hybridmapping.h에 필요한 상수 변수가 정의되어 있지 않을 경우 본인이 이 파일에서 만들어서 사용하면 됨
// 3. 새로운 data structure가 필요하면 이 파일에서 정의해서 쓰기 바람(hybridmapping.h에 추가하면 안됨)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "hybridmapping.h"
// 필요한 경우 헤더 파일을 추가하시오.
#include <stdbool.h>
// 필요한 경우 함수를 추가하시오.
int fdd_read(int ppn, char *pagebuf);
int fdd_write(int ppn, char *pagebuf);
int fdd_erase(int pbn);

struct _address_map {
	int pbn;
	int last_offset;
};

typedef struct _blk_node {
	int pbn;
	struct _blk_node *next;
} BLKnode;

void insert_node(int pbn);
int get_empty_blk();
//
struct _address_map address_mapping_table[DATABLKS_PER_DEVICE];
BLKnode *head;

// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다. 따라서, 첫 번째 ftl_write() 또는 ftl_read()가 호출되기 전에
// file system에 의해 반드시 먼저 호출이 되어야 한다.
//
void ftl_open()
{
	// address mapping table 생성 및 초기화
	for (int i=0; i<DATABLKS_PER_DEVICE; i++){
		address_mapping_table[i].pbn = -1;
		address_mapping_table[i].last_offset = -1;
	}
	
	// free block linked list 생성 및 초기화
	head = NULL;
	for (int i=0; i<BLOCKS_PER_DEVICE; i++){
		int pbn = BLOCKS_PER_DEVICE - 1 - i;
		insert_node(pbn);
	}

 	return;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_read(int lsn, char *sectorbuf)
{
	int lbn = lsn / PAGES_PER_BLOCK;
	int pbn = address_mapping_table[lbn].pbn;
	int last_offset = address_mapping_table[lbn].last_offset;
	char pagebuf[PAGE_SIZE];

	// backward scanning
	for (int i=0; i<=last_offset; i++){
		int ppn = pbn * PAGES_PER_BLOCK + last_offset - i;
		if (fdd_read(ppn, pagebuf) < 0){
			fprintf(stderr, "fdd_read() error\n");
			return;
		}
		int read_lsn;
		memcpy(&read_lsn, pagebuf + SECTOR_SIZE, sizeof(int));
		if (read_lsn == lsn){
			memcpy(sectorbuf, pagebuf, (size_t)SECTOR_SIZE);
			break;
		}
	}
	
	return;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_write(int lsn, char *sectorbuf)
{
	int lbn = lsn / PAGES_PER_BLOCK;
	int pbn = address_mapping_table[lbn].pbn;
	
	if (pbn == -1){ 
		pbn = get_empty_blk();
		address_mapping_table[lbn].pbn = pbn;
	}

	// if the block is full
	if (address_mapping_table[lbn].last_offset == (PAGES_PER_BLOCK -1)){
		// get empty block
		int new_pbn = get_empty_blk();
		int new_last_offset = -1;
		bool is_copied[PAGES_PER_BLOCK];
		for (int i=0; i<PAGES_PER_BLOCK; i++) is_copied[i] = false;
		is_copied[lsn % PAGES_PER_BLOCK] = true;

		// copy latest data
		for (int i=0; i<PAGES_PER_BLOCK; i++){
			int ppn = pbn * PAGES_PER_BLOCK + PAGES_PER_BLOCK -1 -i;
			char pagebuf[PAGE_SIZE];
			if (fdd_read(ppn, pagebuf) < 0){
				fprintf(stderr, "fdd_read() error\n");
				return;
			}
			int read_lsn;
			memcpy(&read_lsn, pagebuf + SECTOR_SIZE, sizeof(int));
			if (!is_copied[read_lsn % PAGES_PER_BLOCK]){
				int new_ppn = new_pbn * PAGES_PER_BLOCK + new_last_offset + 1;
				if (fdd_write(new_ppn, pagebuf) < 0){
					fprintf(stderr, "fdd_write() error\n");
					return;
				}
				new_last_offset++;
			}
			is_copied[read_lsn % PAGES_PER_BLOCK] = true;

		}

		// erase old block
		if (fdd_erase(pbn) < 0){
			fprintf(stderr, "fdd_erase() error\n");
			return;
		}
		// add block node
		insert_node(pbn);
		
		// update address mapping table
		address_mapping_table[lbn].pbn = new_pbn;
		address_mapping_table[lbn].last_offset = new_last_offset;

	}

	pbn = address_mapping_table[lbn].pbn;
	int ppn = pbn * PAGES_PER_BLOCK + address_mapping_table[lbn].last_offset + 1;
	
	// set the buffer
	char pagebuf[PAGE_SIZE];
	memcpy(pagebuf, sectorbuf, (size_t)SECTOR_SIZE);
	memcpy(pagebuf + SECTOR_SIZE, &lsn, sizeof(int));
	
	// write
	if (fdd_write(ppn, pagebuf) < 0){
		fprintf(stderr, "fdd_write() error\n");
		return;
	}

	// update last offset
	address_mapping_table[lbn].last_offset++;

	return;
}

// 
// Address mapping table 등을 출력하는 함수이며, 출력 포맷은 과제 설명서 참조
// 출력 포맷을 반드시 지켜야 하며, 그렇지 않는 경우 채점시 불이익을 받을 수 있음
//
void ftl_print()
{
	printf("lbn pbn last_offset\n");
	for (int i=0; i<DATABLKS_PER_DEVICE; i++){
		printf("%d %d %d\n", i, address_mapping_table[i].pbn, address_mapping_table[i].last_offset);
	}
	return;
}
void insert_node(int pbn){
	// init node
	BLKnode *new_node = (BLKnode *)malloc(sizeof(BLKnode));
	new_node -> pbn = pbn;
	new_node -> next = head;
	head = new_node;
	return;
}
int get_empty_blk(void){ // return pbn
	if (!head) return -1;
	int pbn;
	BLKnode *delete_node;

	delete_node = head;
	head = delete_node -> next;
	pbn = delete_node -> pbn;
	free(delete_node);

	return pbn;
}