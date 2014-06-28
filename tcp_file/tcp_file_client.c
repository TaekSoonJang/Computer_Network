/*소켓 프로그래밍을 위한 헤더파일 선언*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <time.h> 
//avi동영상 파일을 받는다. 시간별로 파일명을 다르게 하기 위해 time 함수를 씀.
//time 함수를 위한 헤더 선언


//패킷을 담을 버퍼사이즈는 1024로 정의
#define BUF_SIZE 1024

//에러 메시지 출력을 위한 함수
void ErrorHandling(char *message);

int main(int argc, char *argv[]){
	int sd;			//클라이언트에서 생성하는 소켓
	FILE *fp;		//클라이언트에서 서버에서 보낸 파일을 write 할 파일의 파일 포인터
	char buf[BUF_SIZE];//클라이언트에서 패킷을 관리한 버퍼 정의
	int read_cnt; //버퍼에서 읽은 파일의 길이.
	struct sockaddr_in serv_adr; //보낼 정보의 목적지 서버 주소를 위한 구조체
	
	if(argc!=3){ //IP와 포트를 인자로 받고 해당 주소로 connect 요청
		printf("Usage: %s <IP> <port>\n",argv[0]);
		exit(1);
	}
	
	
	time_t current_time; //현재 시간을 출력해 주기 위해 시간을 저장할 변수
	time( &current_time);	 //현재의 시간을 저장
	char file_name[BUF_SIZE]; //FILE NAME을 저장할 버퍼 주소
	sprintf(file_name, "%d.pdf",current_time);//파일의 제목을 "현재시간.pdf"로 설정
	printf("%s\n",file_name); //해당하는 파일 이름을 클라이언트에서 출력
	
	fp = fopen(file_name,"wb"); //해당 파일에다가 쓰기위한 파일 포인터를 가져옴.
	sd =socket(PF_INET,SOCK_STREAM,0); //IPv4 프로토콜 체계이며 TCP스트림으로 소켓을 생성하여 endpoint 생성
	
	if(sd==-1) //소켓 에러가 실패했는지 체크
		ErrorHandling("socket() error");

	memset(&serv_adr,0,sizeof(serv_adr)); //커넥트하려는 목적지 서버 주소를 담을 구조체의 초기화
	serv_adr.sin_family=AF_INET; //IPv4의 주소체계를 사용하기 때문에 AF_INET사용
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);// htonl이 필요없이 자동으로 "10.73.43.49" 등으로 입력한 ip주소에 대한 32bit의 주소로 직접 변환을 해준다.
										            // 바이트 오더링도 자동으로 해주기 떄문에 htonl도 필요 없음.
	serv_adr.sin_port=htons(atoi(argv[2])); //접속하려는 포트 번호를 설정(atoi로 숫자로 변환 뒤, htons로 바이트 오더링을 네트워크를 기준으로 맞추어줌).

	if(connect(sd,(struct sockaddr*)&serv_adr,sizeof(serv_adr))==-1) //해당 서버로 커넥트 요청
		ErrorHandling("connect() error!");//커넥트 실패 체크
	else
		puts("Conneected......\n"); //커넥트 성공을 확인하기 위한 출력 메세지

	while((read_cnt=read(sd,buf,BUF_SIZE))!=0) //커넥트가 연결이 되면 해당 스트림을 이용하여 서버에서 오는 파일을 읽음(길이는 read_cnt만큼 읽음)
	{
		fwrite((void*)buf,1,read_cnt,fp); //buf에 담긴 payload를 클라이언트의 file pointer에 차례대로 기록(쓰기)
		//printf("서버로 부터 패킷 받음.\n");
	}
		
	puts("Recieved file data"); //file 전송이 끝나면 확인하기 위한 메시지 출력
	fclose(fp);//파일 포인터를 닫아줌
	close(sd);//해당 소켓을 닫아주고 연결 종료
	return 0;
}

void ErrorHandling(char *message) //에러 메시지 출력을 위한 함수
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}