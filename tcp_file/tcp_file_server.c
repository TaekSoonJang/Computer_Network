#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
//소켓 프로그래밍을 위한 헤더파일 선언

#define BUF_SIZE 1024 
//패킷 정보를 담을 버퍼 사이즈를 1024로 설정

void error_handling(char *message){ //에러 메시지를 출력해줄 함수
	fputs(message, stderr);
	fputc('\n',stderr);
	exit(1);
}

int main(int argc, char *argv[]){
	int serv_sd, clnt_sd; //서버의 소켓, 데이터 교환에 이용활 accept후 생성되는 소켓 정보를 담을 변수 선언
	char buf[BUF_SIZE]; //데이터를 담을 버퍼를 선언
	int read_cnt; //데이터에서 보내고 읽은수를 저장할 변수 선언
	
	struct sockaddr_in serv_adr; //각 소켓에 대한 주소 정보를 담을 변수 선언
	struct sockaddr_in clnt_adr;
	
	socklen_t clnt_adr_sz; //clnt_adr의 크기를 저장할 변수 선언
	FILE * fp; //보낼 파일을 읽기 위한 파일 포인터 변수 선언
	
	if(argc!=2){ //해당 포트에서 잘 연결이 되었는지 확인하는 하수
		printf("Usage : %s <port>\n",argv[0]);
		exit(1);
	}

	fp = fopen("lec11.pdf","rb"); //lec11.pdf의 파일을 전송하기 위해, 파일 포인터로 바이너리 형태로 읽어 온다.
	serv_sd = socket( PF_INET, SOCK_STREAM, 0 ); //IPv4형식의 프로토콜이고, tcp 스트림 통신을 위한 소켓을 생성
	
	if(serv_sd==-1) //소켓 생성의 성공여부 확인을 위한 함수
		error_handling("socket() error");

	memset(&serv_adr,0,sizeof(serv_adr)); //주소 정보를 담을 구조체 변수 초기화
	
	serv_adr.sin_family=AF_INET; //IPv4주소쳬계를 이용하기 때문에 설정
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //소켓을 사용하는 컴퓨터의 ip주소 자동 할당
	serv_adr.sin_port=htons(atoi(argv[1]));//해당하는 포트에 대한 정보 설정

	if(bind(serv_sd,(struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error"); //포트와 소켓이 잘 연결 되었는지 확인
	if(listen(serv_sd,5)==-1)//listen함수로 요청을 기다리고, 해당 요청의 성공여부 확인
		error_handling("listen() error");
		
	clnt_adr_sz=sizeof(clnt_adr); //clnt_adr 의 사이즈 설정
	clnt_sd=accept(serv_sd,(struct sockaddr*)&clnt_adr,&clnt_adr_sz); //accept를 통해 요청상태의 클라이언트와 스트림 연결 생성
		
	if(clnt_sd==-1) //스트림 연결의 성공여부 확인
		error_handling("accept() error");
	else
		printf("Connected client 1 \n");
	
	while(1)
	{
		read_cnt = fread((void*)buf,1,BUF_SIZE, fp); //파일 포인터르 이용하여 바이너리 데이터를 읽어옴
		if( read_cnt < BUF_SIZE){ //보내는 단위가 buf 사이즈보다 작게 남아있으면 그만큼만 읽어들이고 브레이크로 빠져나옴
			write(clnt_sd,buf,read_cnt);
			break;
		}		
		write(clnt_sd,buf,BUF_SIZE); //버퍼 사이즈만큼 데이터를 전송
	}
	
	shutdown(clnt_sd,SHUT_WR);//스트림 소켓의 저옵를 닫아줌(쓰는 소켓만 닫아주고, 읽는 소켓은 열어둠 -> 클라이언트의 마지막 메시지 확인을 위하여)
	read(clnt_sd,buf,BUF_SIZE);//클라이언트에서 보내는 확인 메시지 확인
	printf("Message from client:%s \n",buf);
		
	fclose(fp); //파일 전송이 끝났으므로 , 파일 포인터를 닫아줌
	close(clnt_sd);//스트림 소켓 닫아줌
	close(serv_sd);//서버 소켓을 닫아줌
	return 0;
}