#ifndef _STUDENT_H_
#define _STUDENT_H_

// fixed length record 저장 방식
#define RECORD_SIZE	85	// sid(8) + name(10) + department(12) + address(30) + email(20) + 5*delimiter
#define HEADER_SIZE	8	// #records(4 bytes) + reserved(4 bytes)

// 필요한 경우 'define'을 추가할 수 있음.

enum FIELD {SID=0, NAME, DEPT, ADDR, EMAIL};

typedef struct _STUDENT
{
	char sid[9];		// 학번: NULL 포함 9바이트
	char name[11];	// 이름
	char dept[13];	// 학과
	char addr[31];	// 주소
	char email[21];	// 이메일 주소
} STUDENT;

#endif


int readRecord(FILE *fp, STUDENT *s, int rrn);
void unpack(const char *recordbuf, STUDENT *s);
int writeRecord(FILE *fp, const STUDENT *s, int rrn);
void pack(char *recordbuf, const STUDENT *s);
int writeHeader(FILE *fp, int *record_cnt, int *head);
int readHeader(FILE *fp, int *record_cnt, int *head);
int append(FILE *fp, char *sid, char *name, char *dept, char *addr, char *email);
void search(FILE *fp, enum FIELD f, char *keyval);
void print(STUDENT *s[], int n);
enum FIELD getFieldID(char *fieldname);
int delete(FILE *fp, enum FIELD f, char *keyval);
int insert(FILE *fp, char *id, char *name, char *dept, char *addr, char *email);