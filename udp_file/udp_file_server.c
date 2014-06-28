#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
/*
udp의 패킷로스에 대해 타임아웃을 걸어서
recvfrom의 블락킹함 수 성질을 해결하기 위한
헤더 부분 선언
*/
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
/*
udp에서 패킷로스에 대한 체크를 위해, tcp에서 하는 동작인
시퀀싱과 타임아웃 리트랜스미션을 제공하기 위하여 헤더정보에 시퀀스 정보 추가
udp의 버퍼는 사이즈는 1024로 설정
이중 16바이트는 헤더 정보가 들어가고 1008 사이즈는 데이터가 들어감
*/
#define BUF_SIZE 1024
#define HEADER_SIZE 16
#define DATA_SIZE 1008

static const unsigned int TIMEOUT_SECS = 2; //재전송 기다리는 시간
static const unsigned int MAXTRIES = 5;// 포기할 때까지 반복하는 횟수
unsigned int tries = 0;//보낸 횟수( 시그널 핸들러의 접근을 위해 전역변수 처리)

void CatchAlarm( int ignored ) //타임아웃이 났을 경우 재시도 하는 횟수를 설정
{
	tries += 1;
}//SIGALRM 핸들러


void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n',stderr);
	exit(1);
}

int main(int argc, char *argv[]){
	int serv_sd, clnt_sd; //소켓 생성을 위한 변수 선언
	
	char buf[BUF_SIZE], temp[DATA_SIZE],header[BUF_SIZE], data[DATA_SIZE], init[DATA_SIZE], recv_buf[BUF_SIZE],send_buf[BUF_SIZE];
	//정보를 담을 버퍼사이즈, 처음 udp의 시작을 알리는 init 버퍼, 헤더와 payload를 담을 버퍼 및 보내는 버퍼, 받는 버퍼를 각각 따로 설정
	
	int read_cnt, send_str_len,recv_str_len;
	//각각의 받은 길이, 보낸 길이의 정보를 담을 변수 선언
	
	struct sockaddr_in serv_adr;
	struct sockaddr_in clnt_adr;
	
	//현재 소켓 정보와, 어디로부터 왔는지 목적지 주소를 담을 변수 선언
	socklen_t clnt_adr_sz;	// 주소의 사이즈 정보를 담을 변수 선언
	FILE * fp; //읽고자 하는 파일을 읽어오기 위한 파일 포인터 선언
	

	
	
	//  IP와 포트로의 접속 성공여부확인을 위한 함수
	if(argc!=2){
		printf("Usage : %s <port>\n",argv[0]);
		exit(1);
	}

	//보낼 파일을 읽기 전용으로 열고, 파일포인터에 담음
	fp = fopen("lec11.pdf","rb");
	serv_sd = socket( PF_INET, SOCK_DGRAM, 0 );//IPv4프로토콜 체계로 udp 의 데이터 그램 형식의 소켓을 생성
	
	if(serv_sd==-1)//소켓 생성 여부 확인
		error_handling("socket() error");

	memset(&serv_adr,0,sizeof(serv_adr));//주소정보를 담을 구조체 초기화
	
	serv_adr.sin_family=AF_INET;//IPv4 주소 체계 이용
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //해당 IP주소를 저장
	serv_adr.sin_port=htons(atoi(argv[1]));//포트 정보를 저장

	//소켓과 포트 정보를 연결
	if(bind(serv_sd,(struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
		
	
	//처음에는 발신지를 모르므로, 이닛 패킷을 하나 받음
	clnt_adr_sz=sizeof(clnt_adr);
	recv_str_len = recvfrom(serv_sd, buf, BUF_SIZE,0,(struct sockaddr*)&clnt_adr,&clnt_adr_sz);

	/*
		패킷 로스에 의한 타임아웃 설정을 위하여 시그널 함수를 사요.
	*/
	struct sigaction handler;
	handler.sa_handler = CatchAlarm;
	if( sigfillset(&handler.sa_mask)<0)
		printf("sigfillsetn() failed\n");
		//DieWithSystemMessage("sigfillsetn() failed");
	handler.sa_flags = 0;
	
	if(sigaction(SIGALRM,&handler,0)<0)
		printf("sigaction() failed for SIGALRM\n");
		//DieWithSystemMessage("sigaction() failed for SIGALRM");
	
	//해당 타임아웃의 시간을 설정
	alarm(TIMEOUT_SECS); //타이머 값 설정
	
	int server_seq = 1, client_seq = 0;//시퀀싱을 위한 변수 선언 ,client_seq : client가 다음 번 받을 패킷의 순서, server_seq : 서버가 보낸 패킷 순서
	
	while(1)
	{
		//해당 정보를 담을 버퍼들을 초기화
		memset(buf,0,sizeof(buf));
		memset(header,0,sizeof(header));
		memset(data,0,sizeof(data));
		memset(temp,0,sizeof(temp));
		
		read_cnt = fread((void*)temp,1,DATA_SIZE, fp); //temp버퍼에 데이터 사이즈만큼 읽어와서 데이터를 담음

		sprintf( buf, "%d", server_seq ); //버퍼의 16바이에 해당하는 앞쪽에는 서버가 보내는 시퀀싱 넘버를 헤더정보로 넣어줌
		
		if( read_cnt < DATA_SIZE ) //보내고자 하는 파일이 마지막 부분이라면 해당 파일을 보내주고 종료
		{ 
			for( int i=HEADER_SIZE; i<HEADER_SIZE+read_cnt; ++i )
			{
				buf[i] = temp[i-HEADER_SIZE]; //버퍼의 데이터가 들어가야할 부분에 temp에 담아두었던 데이터 복사
			}
	
			while(1)
			{
				int flag = 0; //재전송을 확인하는 flag 변수
				while(1)
				{
					send_str_len = sendto(serv_sd, buf, HEADER_SIZE+read_cnt, 0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
					//클라이언트로 해당 버퍼를 전송
					if( send_str_len == -1 )
					{
						//printf("서버에서 패킷을 전송하지 못했음 \n");
						continue;
					}
					else 
						break;
				}
				
				while(1)
				{
					alarm(TIMEOUT_SECS); //타이머 값 설정
					recv_str_len = recvfrom(serv_sd, recv_buf, BUF_SIZE,0,(struct sockaddr*)&clnt_adr,&clnt_adr_sz);
					//클라이언트로부터 ack를 잘 받았는지 확인
					//만약 타임아웃이 나면 다시 재전송을 요청.
					if( errno == EINTR && recv_str_len == -1)
					{
						printf("server time out!!");
						flag = 1;
						alarm(0); //타이머 값 설정
						break;
					}
					if( recv_str_len == -1 )
					{
						//printf( "client로부터 seq못받음 \n");
						continue;
					}
					else
						break;
				}
				if( flag )
					continue;//타임아웃이 났다면 다시 위로가서 재전송을 함.
				
				if( !strcmp(recv_buf,"resend") ) //역시 resend라는 값이 왔다면 재전송.
				{
					continue;
				}
				else //잘 받았으면
				{	
					client_seq = atoi(recv_buf); //받은 헤더에서 시퀀싱 넘버를 보고 , 갱신
					//printf("마지막 부분 %d에서부터 %d까지 패킷을 전송 \n",server_seq, client_seq -1 );
					server_seq = client_seq; //서버에서 보낼 시퀀싱 넘버 갱신
					break;
				}
			}
			//전송 끝
			break;
		}
		
		
		for( int i=HEADER_SIZE; i<HEADER_SIZE+DATA_SIZE; ++i )
		{
			buf[ i ] = temp[ i-HEADER_SIZE ];//버퍼의 데이터가 들어가야할 부분에 temp에 담아두었던 데이터 복사
		}
		
		while(1)
		{
			int flag = 0;//재전송을 확인하는 flag 변수
			
			while(1)
			{
				send_str_len = sendto(serv_sd, buf, BUF_SIZE, 0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
				//클라이언트로 해당 버퍼를 전송
				if( send_str_len == -1 )
				{
				//	printf("서버에서 패킷을 전송하지 못했음 \n");
					continue;
				}
				else 
				{
					//memset(chk_send,0,sizeof(chk_send));
		//			printf("client로 패킷 전송 잘했음\n");
					//strcpy(chk_send, buf);
					break;
				}
			}
			
			while(1)
			{
				alarm(TIMEOUT_SECS); //타이머 값 설정
				recv_str_len = recvfrom(serv_sd, recv_buf, BUF_SIZE,0,(struct sockaddr*)&clnt_adr,&clnt_adr_sz);
				//클라이언트로부터 ack를 잘 받았는지 확인
				//만약 타임아웃이 나면 다시 재전송을 요청
				if ( errno == EINTR && recv_str_len == -1)
				{
					printf("server time out\n");
					flag = 1;
					alarm(0); //타이머 값 설정
			//		printf(" intrrupt server recv \n");
					break;
				}
				if( recv_str_len == -1 )
				{
			//		printf( "client로부터 seq못받음 \n");
					continue;
				}
				else
				{
			//		printf("client로부터 seq잘 받음\n");
					//strcpy(chk_recv, buf);
					break;
				}
			}
			if( flag == 1)//타임아웃이 났다면 다시 위로가서 재전송을 함.
				continue;
				
			if( !strcmp(recv_buf,"resend") )//역시 resend라는 값이 왔다면 재전송.
			{
		//		printf(" resend 해주세요 from server \n");
				continue;
			}
			else //잘 받았으면
			{	
				client_seq = atoi(recv_buf);//받은 헤더에서 시퀀싱 넘버를 보고 , 갱신
				/*
			//	printf("%d 에서부터 %d까지 패킷을 전송 \n",server_seq, client_seq -1 );
				for( int i=0; i<BUF_SIZE; ++i )
				{
				//	printf("%02x",chk_send[i]);
				}
		//		printf("\n\n\n");
		*/
				server_seq = client_seq;
				break;
			}
		}
	}
	
	alarm(0); //타임아웃 설정 해제
	
	printf("server end\n"); //서버에서 전송이 끝났음을 알리는 메시지 출력
	fclose(fp);// 파일 포인터를 닫음
	close(serv_sd);//소켓을 닫음
	return 0;
}