#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "flash.h"
// 필요한 경우 헤더파일을 추가한다

FILE *flashmemoryfp;	// fdevicedriver.c에서 사용

char sectorbuf[SECTOR_SIZE];
char pagebuf[PAGE_SIZE];
char *blockbuf;

int ftl_makefile(char *argv[]);
int ftl_write(char *argv[]);
int ftl_read(char *argv[]);
int ftl_erase(char *argv[]);
int ftl_update(char *argv[]);

int is_page_empty(const int ppn);
int is_ppn_valid(const char *pathname, const char *ppn);
int is_pbn_valid(const char *pathname, const char *pbn);

int find_empty_block(const char *pathname);

//
// 이 함수는 FTL의 역할 중 일부분을 수행하는데 물리적인 저장장치 flash memory에 Flash device driver를 이용하여 데이터를
// 읽고 쓰거나 블록을 소거하는 일을 한다 (강의자료 참조).
// flash memory에 데이터를 읽고 쓰거나 소거하기 위해서 fdevicedriver.c에서 제공하는 인터페이스를
// 호출하면 된다. 이때 해당되는 인터페이스를 호출할 때 연산의 단위를 정확히 사용해야 한다.
// 읽기와 쓰기는 페이지 단위이며 소거는 블록 단위이다 (주의: 읽기, 쓰기, 소거를 하기 위해서는 반드시 fdevicedriver.c의 함수를 사용해야 함)
// 갱신은 강의자료의 in-place update를 따른다고 가정한다.
// 스페어영역에 저장되는 정수는 하나이며, 반드시 binary integer 모드로 저장해야 한다.
//
int main(int argc, char *argv[])
{	
	// 1. "c": flash memory emulator
	if (argc == 4 && strcmp(argv[1], "c")==0) {
		if (ftl_makefile(argv)<0){
			exit(1);
		}
		exit(0);
	}
	// 2. "w": 페이지 쓰기
	//		pagebuf의 sector와 spare에 각각 입력된 데이터를 정확히 저장한 후 fdd_write()를 호출한다.
	if (argc == 6 && strcmp(argv[1], "w")==0){
		if (ftl_write(argv)<0){
			exit(1);
		}
		exit(0);
	}
	// 3. "r": 페이지 읽기
	//		pagebuf를 인자로 사용하여 인터페이스를 호출, 즉 fdd_read()를 호출하고 페이지를 읽어온 후
	//		sector data와 spare data를 분리한다.
	if (argc == 4 && strcmp(argv[1], "r")==0){
		if (ftl_read(argv)<0){
			exit(1);
		}
		exit(0);
	}
	// 4. "e": 블록 소거
	//		입력한 pbn에 해당하는 block의 데이터를 삭제한다.
	if (argc == 4 && strcmp(argv[1], "e")==0){
		if (ftl_erase(argv)<0){
			exit(1);
		}
		exit(0);
	}
	// 5. in-place update
	//		해당 페이지에 데이터가 이미 존재할 경우 in-place update를 수행한다.
	//		빈 블록을 할당 받아 기존 데이터를 복사한 후, 새로운 데이터를 추가하고 기존 데이터를 재복사한다.
	if (argc == 6 && strcmp(argv[1], "u")==0){
		if (ftl_update(argv)<0){
			exit(1);
		}
		exit(0);
	}
	// 정의되지 않은 명령어
	exit(1);
}
// 1. flash memory emulator
int ftl_makefile(char *argv[]){
	// file open
	if ((flashmemoryfp = fopen(argv[2], "wb+"))==NULL){
		return -1;
	}
	int blk_num;
	// 0xff로 초기화
	if ((blk_num = atoi(argv[3]))>0){
		for (int i=0; i<blk_num; i++){
			if (fdd_erase(i)<0) exit(1);
		}
	} else {
		fprintf(stderr, "Error: Block number is positive\n");
		return -1;
	}
	return 0;
}
// 2. write
int ftl_write(char *argv[]){
	// open the file
	if ((flashmemoryfp = fopen(argv[2], "rb+"))==NULL){
		fprintf(stderr, "Error: open error\n");
		return -1;			
	}
	// validate the page number
	if (is_ppn_valid(argv[2], argv[3])!=1){
		return -1;
	}
	// is the page empty
	int ppn = atoi(argv[3]);
	if (is_page_empty(ppn)!=1){
		fprintf(stderr, "Error: Data already exist\n");
		return -1;
	}
	// size of data
	if (strlen(argv[4])>SECTOR_SIZE){
		fprintf(stderr, "Error: Data too long\n");
		return -1;
	}
	// set buffer
	strcpy(sectorbuf, argv[4]);
	int spare=atoi(argv[5]);

	memcpy(pagebuf, sectorbuf, strlen(sectorbuf));
	memcpy(pagebuf+SECTOR_SIZE, &spare, (size_t)4);	
	// write
	if (fdd_write(ppn, pagebuf)<0){
		fprintf(stderr, "Error: write error");
		return -1;
	}
	return 0;
}
// 3. read
int ftl_read(char *argv[]){
	// open the file
	if ((flashmemoryfp = fopen(argv[2], "rb"))==NULL){
		fprintf(stderr, "Error: open error\n");
		return -1;
	}
	// validate the page number
	if (is_ppn_valid(argv[2], argv[3])!=1){
		return -1;
	}
	// read
	int ppn=atoi(argv[3]);
	if (fdd_read(ppn, pagebuf)<0){
		fprintf(stderr, "Error: read error\n");
		return -1;
	}
	// print: sector
	for (int i=0;i<SECTOR_SIZE;i++){
		if (pagebuf[i]==(char)0xff)
			break;
		else printf("%c", pagebuf[i]);
	}
	printf(" ");

	// print: spare
	int spare;
	memcpy(&spare, pagebuf+SECTOR_SIZE, (size_t)4);
	printf("%d\n", spare);
	
	return 0;
}
// 4. erase block
int ftl_erase(char *argv[]){
	// open the file
	if ((flashmemoryfp = fopen(argv[2], "rb+"))==NULL){
		fprintf(stderr, "Error: open error\n");
		return -1;
	}
	// validate the block number
	if (is_pbn_valid(argv[2], argv[3])!=1){
		return -1;
	}
	// erase
	int pbn=atoi(argv[3]);
	if (fdd_erase(pbn)<0){
		fprintf(stderr, "Error: erase error\n");
		return -1;
	}
	return 0;
}
// 5. in-place update
int ftl_update(char *argv[]){
	int cnt_read=0, cnt_write=0, cnt_erase=0;
	// open the file
	if ((flashmemoryfp = fopen(argv[2], "rb+"))==NULL){
		fprintf(stderr, "Error: open error\n");
		return -1;
	}
	// validate the page number
	if (is_ppn_valid(argv[2], argv[3])!=1){
		return -1;
	}
	// is the page empty
	int ppn=atoi(argv[3]);
	if (is_page_empty(ppn)!=0){
		// 빈 페이지일 경우 update할 데이터가 존재하지 않으므로 종료
		return -1;
	}
	// size of data
	if (strlen(argv[4])>SECTOR_SIZE){
		fprintf(stderr, "Error: Data too long\n");
		return -1;
	}
	// find the empty block
	int temp_pbn;
	if ((temp_pbn=find_empty_block(argv[2]))<0){
		return -1;
	}
	// copy
	int pbn=(int)ppn/PAGE_NUM;
	for (int i=0; i<PAGE_NUM; i++){
		if (i==ppn%PAGE_NUM){
			continue;
		}
		if (is_page_empty(pbn*PAGE_NUM+i)){
			cnt_read++;
			continue;
		}
		if (fdd_read(pbn*PAGE_NUM+i, pagebuf)<0){
			fprintf(stderr, "Error: copy error\n");
			return -1;
		} else {
			cnt_read++;
		}
		if (fdd_write(temp_pbn*PAGE_NUM+i, pagebuf)<0){
			fprintf(stderr, "Error: paste error\n");
			return -1;
		} else {
			cnt_write++;
		}
	}
	// erase
	if (fdd_erase(pbn)<0){
		fprintf(stderr, "Error: erase error\n");
		return -1;
	} else {
		cnt_erase++;
	}
	// set the buffer
	strcpy(sectorbuf, argv[4]);
	int spare=atoi(argv[5]);	

	memcpy(pagebuf, sectorbuf, strlen(sectorbuf));
	memcpy(pagebuf+SECTOR_SIZE, &spare, (size_t)4);	
	// write(update)
	if (fdd_write(ppn, pagebuf)<0){
		fprintf(stderr, "Error: update error\n");
		return -1;
	} else {
		cnt_write++;
	}
	// re-copy
	for (int i=0; i<PAGE_NUM; i++){
		if (i==ppn%PAGE_NUM){
			continue;
		}
		if (is_page_empty(temp_pbn*PAGE_NUM+i)){
			continue;
		}
		if (fdd_read(temp_pbn*PAGE_NUM+i, pagebuf)<0){
			fprintf(stderr, "Error: read error\n");
			return -1;
		} else {
			cnt_read++;
		}
		if (fdd_write(pbn*PAGE_NUM+i, pagebuf)<0){
			fprintf(stderr, "Error: re-copy error\n");
			return -1;
		} else {
			cnt_write++;
		}
	}
	// erase
	if (fdd_erase(temp_pbn)<0){
		fprintf(stderr, "Error: erase error\n");
		return -1;
	} else {
		cnt_erase++;
	}
	// print
	printf("#reads=%d #writes=%d #erases=%d\n", cnt_read, cnt_write, cnt_erase);
	return 0;
}
int is_page_empty(const int ppn){
	// RETURN VALUE
	//  1: empty
	//  0: already exist
	// -1: error
	if (fdd_read(ppn, pagebuf)<0){
		fprintf(stderr, "Error: read error\n");
		return -1;
	}
	for (int i=0 ; i<PAGE_SIZE ; i++){
		if (pagebuf[i]!=(char)0xff){ 
			return 0;
		}
	}
	return 1;
}
int is_ppn_valid(const char *pathname, const char *ppn){
	// RETURN VALUE
	//  1: valid
	//  0: invalid
	// -1: error

	struct stat sb;

	if (atoi(ppn)<0){
		fprintf(stderr, "Error: Page number is non-negative\n");
		return -1;
	} else if (atoi(ppn)==0 && strcmp(ppn, "0")!=0){
		fprintf(stderr, "Error: Invalid number\n");
		return -1;
	}
	if (stat(pathname, &sb)<0){
		fprintf(stderr, "Error: stat error\n");
		return -1;
	}
	if (sb.st_size / PAGE_SIZE <= atoi(ppn)){
		fprintf(stderr, "Error: Page number is out of range\n");
		return 0;
	}
	return 1;
}
int is_pbn_valid(const char *pathname, const char *pbn){
	// RETURN VALUE
	//  1: valid
	//  0: invalid
	// -1: error

	struct stat sb;

	if (atoi(pbn)<0){
		fprintf(stderr, "Error: Page number is non-negative\n");
		return -1;
	} else if (atoi(pbn)==0 && strcmp(pbn, "0")!=0){
		fprintf(stderr, "Error: Invalid number\n");
		return -1;
	}
	if (stat(pathname, &sb)<0){
		fprintf(stderr, "Error: stat error\n");
		return -1;
	}
	if (sb.st_size / BLOCK_SIZE <= atoi(pbn)){
		fprintf(stderr, "Error: Page number is out of range\n");
		return 0;
	}
	return 1;
}
int find_empty_block(const char *pathname){
	// RETURN VALUE
	//  >= 0: block offset
	//    -1: error

	struct stat sb;
	int blk_num;
	
	if (stat(pathname, &sb)<0){
		fprintf(stderr, "Error: stat error\n");
		return -1;
	}
	blk_num = sb.st_size/BLOCK_SIZE;
	for (int i=blk_num-1; i>=0; i--){
		for (int j=i*PAGE_NUM; j<i*PAGE_NUM+PAGE_NUM; j++){
			if (is_page_empty(j)!=1)
				break;
			else if (j==i*PAGE_NUM+PAGE_NUM-1)
				return i;
		}
	}
	return -1;
}