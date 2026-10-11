#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>	
#include <stdbool.h>	// 필요한 header file 추가 가능
#include "student.h"

//
// 함수 readRecord()는 학생 레코드 파일에서 주어진 rrn에 해당하는 레코드를 읽어서 
// recordbuf에 저장하고, 이후 unpack() 함수를 호출하여 학생 타입의 변수에 레코드의
// 각 필드값을 저장한다. 성공하면 1을 그렇지 않으면 0을 리턴한다.
// unpack() 함수는 recordbuf에 저장되어 있는 record에서 각 field를 추출하는 일을 한다.
//
int readRecord(FILE *fp, STUDENT *s, int rrn){
	char recordbuf[RECORD_SIZE];
	fseek(fp, (long)HEADER_SIZE+RECORD_SIZE*rrn, SEEK_SET);
	if (!fread((char *)recordbuf, (size_t)RECORD_SIZE, (size_t)1, fp))
		return 0;
	
	unpack(recordbuf, s);
	return 1;
}
void unpack(const char *recordbuf, STUDENT *s){
	char record[RECORD_SIZE];
	strcpy(record, recordbuf);

	strcpy(s->sid, strtok(record, "#"));
	strcpy(s->name, strtok(NULL, "#"));
	strcpy(s->dept, strtok(NULL, "#"));
	strcpy(s->addr, strtok(NULL, "#"));
	strcpy(s->email, strtok(NULL, "#"));
}

//
// 함수 writeRecord()는 학생 레코드 파일에 주어진 rrn에 해당하는 위치에 recordbuf에 
// 저장되어 있는 레코드를 저장한다. 이전에 pack() 함수를 호출하여 recordbuf에 데이터를 채워 넣는다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//
int writeRecord(FILE *fp, const STUDENT *s, int rrn){
	// set record buffer
	char recordbuf[RECORD_SIZE];
	pack(recordbuf, s);

	// write record
	fseek(fp, (long)HEADER_SIZE+RECORD_SIZE*rrn, SEEK_SET);
	if (!fwrite((char *)recordbuf, (size_t)RECORD_SIZE, (size_t)1, fp))
		return 0;

	return 1;
}
void pack(char *recordbuf, const STUDENT *s){
	sprintf(recordbuf, "%s#%s#%s#%s#%s#", s->sid, s->name, s->dept, s->addr, s->email);
}
int writeHeader(FILE *fp, int *record_cnt, int *head){
	char headerbuf[HEADER_SIZE];

	memcpy((char *)headerbuf, (int *)record_cnt, (size_t)4);
	memcpy((char *)headerbuf+4, (int *)head, (size_t)4);

	if (fseek(fp, (long)0, SEEK_SET) < 0){
		perror("fseek");
		return 0;
	}
	if (!fwrite((char *)headerbuf, (size_t)HEADER_SIZE, (size_t)1, fp)){
		perror("fwrite");
		return 0;
	}
	return 1;
}
int readHeader(FILE *fp, int *record_cnt, int *head){
	char headerbuf[HEADER_SIZE];

	if (fseek(fp, (long)0, SEEK_SET) < 0){
		perror("fseek");
		return 0;
	}
	if (!fread((char *)headerbuf, (size_t)HEADER_SIZE, (size_t)1, fp)){
		perror("fread");
		return 0;
	}

	memcpy((int *)record_cnt, (char *)headerbuf, (size_t)4);
	memcpy((int *)head, (char *)headerbuf+4, (size_t)4);

	return 1;
}

//
// 함수 appendRecord()는 학생 레코드 파일에 새로운 레코드를 append한다.
// 레코드 파일에 레코드가 하나도 존재하지 않는 경우 (첫 번째 append)는 header 레코드 다음에
// 첫 번째 레코드를 저장한다. 당연히 레코드를 append를 할 때마다 header 레코드에 대한 수정이 뒤따라야 한다.
// 함수 append()는 내부적으로 writeRecord() 함수를 호출하여 레코드 저장을 해결한다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//
int append(FILE *fp, char *sid, char *name, char *dept, char *addr, char *email){
	// read header
	int record_cnt;
	int head;

	if (!readHeader(fp, &record_cnt, &head)){
		perror("readHeader");
		return 0;
	}

	// set record
	STUDENT record = {"", "", "", "", ""};

	if (strlen(sid) > (size_t)8) return 0;
	else strcpy(record.sid, sid);

	if (strlen(name) > (size_t)10) return 0;
	else strcpy(record.name, name);

	if (strlen(dept) > (size_t)12) return 0;
	else strcpy(record.dept, dept);

	if (strlen(addr) > (size_t)30) return 0;
	else strcpy(record.addr, addr);

	if (strlen(email) > (size_t)20) return 0;
	else strcpy(record.email, email);

	// write record
	if (!writeRecord(fp, &record, record_cnt)){
		perror("writeRecord");
		return 0;
	}
	record_cnt++;

	// update header
	if (!writeHeader(fp, &record_cnt, &head)){
		perror("writeHeader");
		return 0;
	}

	return 1;
}

//
// 학생 레코드 파일에서 검색 키값을 만족하는 레코드가 존재하는지를 sequential search 기법을 
// 통해 찾아내고, 이를 만족하는 모든 레코드의 내용을 출력한다. 검색 키는 학생 레코드를 구성하는
// 학번, 이름, 학과 필드만 사용한다 (명세서 참조). 내부적으로 readRecord() 함수를 호출하여 sequential search를 수행한다.
// 검색 결과를 출력할 때 반드시 print() 함수를 사용한다. (반드시 지켜야 하며, 그렇지
// 않는 경우 채점 프로그램에서 자동적으로 틀린 것으로 인식함)
// 과제 5에서는 삭제 레코드가 파일에 존재할 수 있으므로 이를 고려하여 검색 함수를 수정해야 한다.
//
void search(FILE *fp, enum FIELD f, char *keyval){
	int record_cnt;
	int head;
	char headerbuf[HEADER_SIZE];

	// set the number of record
	if (!readHeader(fp, &record_cnt, &head)){
		perror("readHeader");
	}

	// search 
	STUDENT **print_records = (STUDENT **)calloc((size_t)record_cnt, sizeof(STUDENT *));
	int print_cnt=0;

	int cnt = 0;
	for (int rrn=0 ; cnt != record_cnt; rrn++){
		// read record
		char recordbuf[RECORD_SIZE];
		fseek(fp, (long)HEADER_SIZE+RECORD_SIZE*rrn, SEEK_SET);
		fread((char *)recordbuf, (size_t)RECORD_SIZE, (size_t)1, fp);
		

		// skip deleted record
		if (recordbuf[0] == '*'){
			continue;
		} else cnt++;

		STUDENT s; 
		unpack(recordbuf, &s);

		int is_print = 0;
		switch (f){
			case SID:
				if (!strcmp(s.sid, keyval)) is_print = 1;
				break;
			case NAME:
				if (!strcmp(s.name, keyval)) is_print = 1;
				break;
			case DEPT:
				if (!strcmp(s.dept, keyval)) is_print = 1;
				break;	
		}
		if (is_print){
			print_records[print_cnt] = (STUDENT *)calloc((size_t)1, sizeof(STUDENT));
			memcpy(print_records[print_cnt], &s, sizeof(STUDENT));
			print_cnt++;
		}		
	}	
	print(print_records, print_cnt);
	
	for (int i=0; i<record_cnt; i++)
		free(print_records[i]);
	free(print_records);
}
void print(STUDENT *s[], int n)
{
	printf("#records = %d\n", n);
	for(int i = 0; i < n; i++)
	{
		printf("%s#%s#%s#%s#%s#\n", s[i]->sid, s[i]->name, s[i]->dept, s[i]->addr, s[i]->email);
	}
}

//
// 레코드의 필드명을 enum FIELD 타입의 값으로 변환시켜 준다.
// 예를 들면, 사용자가 수행한 명령어의 인자로 "NAME"이라는 필드명을 사용하였다면 
// 프로그램 내부에서 이를 NAME(=1)으로 변환할 필요성이 있으며, 이때 이 함수를 이용한다.
//
enum FIELD getFieldID(char *fieldname){
	if (!strcmp(fieldname, "SID"))
		return SID;
	else if (!strcmp(fieldname, "NAME"))
		return NAME;
	else if (!strcmp(fieldname, "DEPT"))
		return DEPT;
	else if (!strcmp(fieldname, "ADDR"))
		return ADDR;
	else if (!strcmp(fieldname, "EMAIL"))
		return EMAIL;
}
//
// 학생 레코드 파일에서 조건을 만족하는 레코드를 찾아서 이것을 삭제한다.
// 참고로, 검색 조건은 학번(SID), 이름(NAME), 학과(DEPT) 중 하나만을 사용한다. 
// 또한, 삭제되는 레코드가 존재하면 이것을 삭제 레코드 리스트에 추가한다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//
int delete(FILE *fp, enum FIELD f, char *keyval){
	int record_cnt;
	int head;

	// read header
	if (!readHeader(fp, &record_cnt, &head)){
		perror("readHeader");
		return 0;
	}

	int delete_cnt = 0;
	int cnt = 0;
	// sequential search
	for (int rrn = 0; cnt != record_cnt; rrn++){
		// read record;
		char recordbuf[RECORD_SIZE];
		fseek(fp, (long)HEADER_SIZE+RECORD_SIZE*rrn, SEEK_SET);
		if (!fread((char *)recordbuf, (size_t)RECORD_SIZE, (size_t)1, fp))
			return 0;
		
		if (recordbuf[0] == '*'){
			continue;
		} else cnt++;

		STUDENT record;
		unpack(recordbuf, &record);
		// key
		bool is_deleted = false;
		switch (f){
			case SID:
				if (!strcmp(record.sid, keyval)) is_deleted = true;
				break;
			case NAME:
				if (!strcmp(record.name, keyval)) is_deleted = true;
				break;
			case DEPT:
				if (!strcmp(record.dept, keyval)) is_deleted = true;
				break;
		}
		// delete
		if (is_deleted){
			char recordbuf[RECORD_SIZE] = "*";
			memcpy((char *)recordbuf+1, (int *)&head, (size_t)4);
			head = rrn;
			if (fseek(fp, (long)HEADER_SIZE+RECORD_SIZE*rrn, SEEK_SET) < 0){
				perror("fseek");
				return 0;
			}
			if (!fwrite((char *)recordbuf, (size_t)RECORD_SIZE, (size_t)1, fp)){
				perror("fwrite");
				return 0;
			}
			delete_cnt++;
		}
	}
	// update header
	record_cnt -= delete_cnt;

	if (!writeHeader(fp, &record_cnt, &head)){
		perror("writeHeader");
		return 0;
	}

	return 1;
}

//
// 학생 레코드 파일에 새로운 레코드를 추가한다. 삭제 레코드가
// 존재하면 반드시 삭제 레코드들 중 하나에 새로운 레코드를 저장한다. (삭제 레코드의 선택은 과제 설명서 참조)
// 반면, 삭제 레코드가 하나도 존재하지 않으면 append 형태로 새로운 레코드를 추가한다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//
int insert(FILE *fp, char *id, char *name, char *dept, char *addr, char *email){
	int record_cnt;
	int head;

	if (!readHeader(fp, &record_cnt, &head)){
		perror("readHeader");
		return 0;
	}

	if (head == -1){
		if (!append(fp, id, name, dept, addr, email)){
			perror("append");
			return 0;
		}
		return 1;
	}
	int write_rrn = head;
	int new_head;
	char recordbuf[RECORD_SIZE];

	if (fseek(fp, (long)HEADER_SIZE+RECORD_SIZE*write_rrn, SEEK_SET) < 0){
		perror("fseek");
		return 0;
	}
	if (!fread((char *)recordbuf, (size_t)RECORD_SIZE, (size_t)1, fp)){
		perror("fread");
		return 0;
	}
	memcpy((int *)&new_head, (char *)recordbuf+1, (size_t)4);

	// write record
	STUDENT record = {0};
	
	if (strlen(id) > (size_t)8) return 0;
	else strcpy(record.sid, id);

	if (strlen(name) > (size_t)10) return 0;
	else strcpy(record.name, name);

	if (strlen(dept) > (size_t)12) return 0;
	else strcpy(record.dept, dept);

	if (strlen(addr) > (size_t)30) return 0;
	else strcpy(record.addr, addr);

	if (strlen(email) > (size_t)20) return 0;
	else strcpy(record.email, email);

	if (!writeRecord(fp, &record, write_rrn)){
		perror("writeRecord");
		return 0;
	}

	// update header
	record_cnt++;

	if (!writeHeader(fp, &record_cnt, &new_head)){
		perror("writeHeader");
		return 0;
	}

	return 1;
}

void main(int argc, char *argv[])
{
	// 모든 file processing operation은 C library나 system call을 사용한다.
	// C library인 경우 FILE *fp를, sysem call인 경우 int fd와 같은 변수를 선택적으로 사용함
	// readRecord()와 같이 FILE *fp 인자가 쓰여져 있는 함수에도 똑같이 선택적으로 사용함
	//	
	char filename[256];
	
	if (!(argc == 4 || argc == 8)){
		fprintf(stderr, "Error: Invalid command\n");
		exit(1);
	}
	if (!strcmp(argv[1], "-a") || !strcmp(argv[1], "-s"))
		strcpy(filename, argv[2]);
	else if (!strcmp(argv[1], "-d") || !strcmp(argv[1], "-i"))
		strcpy(filename, argv[2]);
	else {
		fprintf(stderr, "Error: Invalid command\n");
		exit(1);
	}

	//
	// append나 search 명령어에서 입력으로 주어지는 <record_file_name> 이름으로 파일을 생성한다.
	// 당연히 두 명령어 중 하나가 최초로 실행될 때 <record_file_name>파일을 최초로 생성하고,
	// 이후에는 디스크에 이 파일이 존재하기 때문에 새로 생성해서는 안된다. 즉, 이 파일이 디스크에 존재하지 않으면
	// 최초로 생성하고, 그렇지 않으면 생성하지 않고 skip 한다.
	// 최초로 파일을 생성할 때 header record (크기=8B)를 저장한다.
	//

	FILE *fp;	// or int fd;	
	if (access(filename, F_OK) < 0){
		// create file
		if ((fp = fopen(filename, "w+b")) == NULL){
			fprintf(stderr, "Error: cannot open the file\n");
			exit(1);
		}
		// add header
		int record_num = 0;
		int head = -1;

		if (!writeHeader(fp, &record_num, &head)){
			perror("writeHeader");
			fclose(fp);
			exit(1);
		}
	} else {
		if ((fp = fopen(filename, "r+b")) == NULL){
			fprintf(stderr, "Error: cannot open the file\n");
			exit(1);
		}
	}

	/* APPEND */
	if (argc == 8 && !strcmp(argv[1], "-a")){
		if (!append(fp, argv[3], argv[4], argv[5], argv[6], argv[7])){
			perror("append");
			fclose(fp);
			exit(1);
		}
	}
	/* INSERT */
	else if (argc == 8 && !strcmp(argv[1], "-i")){
		if (!insert(fp, argv[3], argv[4], argv[5], argv[6], argv[7])){
			perror("insert");
			fclose(fp);
			exit(1);
		}
	}
	/* DELETE */
	else if (argc == 4 && !strcmp(argv[1], "-d")){
		char *fieldname = strtok(argv[3], "=");
		char *keyvalue = strtok(NULL, "=");

		if (!delete(fp, getFieldID(fieldname), keyvalue)){
			perror("delete");
			fclose(fp);
			exit(1);
		}
	}
	/* SEARCH */
	else if (argc == 4 && !strcmp(argv[1], "-s")){
		char *fieldname = strtok(argv[3], "=");
		char *keyvalue = strtok(NULL, "=");

		search(fp, getFieldID(fieldname), keyvalue);
	}
	else {
		fprintf(stderr, "Error: Invalid command\n");
	}
	fclose(fp);
	exit(0);
}

