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

void ErrorHandling(char *message);
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

int main(int argc, char *argv[]){
	int sd; //소켓 생성을 위한 변수 선언
	FILE *fp; //쓰고자 하는 파일을 읽어오기 위한 파일 포인터 선언
	char buf[BUF_SIZE], init[BUF_SIZE], header[BUF_SIZE], data[DATA_SIZE], recv_buf[BUF_SIZE],send_buf[BUF_SIZE];
	//정보를 담을 버퍼사이즈, 처음 udp의 시작을 알리는 init 버퍼, 헤더와 payload를 담을 버퍼 및 보내는 버퍼, 받는 버퍼를 각각 따로 설정
	
	int read_cnt, send_str_len, recv_str_len;
	//각각의 받은 길이, 보낸 길이의 정보를 담을 변수 선언
	
	struct sockaddr_in serv_adr, from_adr;
	//현재 소켓 정보와, 어디로부터 왔는지 목적지 주소를 담을 변수 선언
	
	socklen_t adr_sz;
	// 주소의 사이즈 정보를 담을 변수 선언
	
	
	//  IP와 포트로의 접속 성공여부확인을 위한 함수
	if(argc!=3){
		printf("Usage: %s <IP><port>\n",argv[0]);
		exit(1);
	}

	//받을 파일을 쓰기 전용으로 열고, 파일포인터에 담음
	fp = fopen("receive_udp.pdf","wb");
	
	sd =socket(PF_INET,SOCK_DGRAM,0);//IPv4프로토콜 체계로 udp 의 데이터 그램 형식의 소켓을 생성
	
	if(sd==-1) //소켓 생성 여부 확인
		ErrorHandling("socket() error");

	memset(&serv_adr,0,sizeof(serv_adr)); //주소정보를 담을 구조체 초기화
	serv_adr.sin_family=AF_INET; //IPv4 주소 체계 이용
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]); //해당 IP주소를 저장
	serv_adr.sin_port=htons(atoi(argv[2])); //포트 정보를 저장
	
	strcpy( init, "init" ); //udp에서 init을 보내 서버가 보내줄 위치를 알리기 위한 init 패킷

	while(1)
	{
		send_str_len = sendto( sd, init, BUF_SIZE,0,(struct sockaddr*)&serv_adr,sizeof(serv_adr) );
		//서버로 이닛 패킷을 보냄
		
		if( send_str_len == -1 ) //이닛 패킷의 성공 여부 확인
		{
			
			printf("init패킷을 전송하지 못함(재전송요망)\n");
			continue;
		}
		else
		{
			printf("init 패킷 전송을 성공. 이제부터 데이터 주고받자\n");
			break;
		}
	}
	
	
	int client_seq = 1, server_seq=0; //시퀀싱을 위한 변수 선언 ,client_seq : client가 받아야할 패킷 순서, server_seq : 서버가 보낸 패킷 순서
	
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
		
	while(1)
	{	
		while(1)//server에서 보내는 데이터를 받음
		{
			//해당 정보를 담을 버퍼들을 초기화
			memset(header, 0, sizeof(header));
			memset(data, 0, sizeof(data));
			memset(recv_buf, 0, sizeof(recv_buf));
			
			adr_sz = sizeof(from_adr); //목적지 정보의 사이즈를 저장
			alarm(TIMEOUT_SECS); //타이머 값 설정
			
			read_cnt = recvfrom(sd,recv_buf,BUF_SIZE,0,(struct sockaddr*)&from_adr,&adr_sz);
			//recv_buf에 버퍼 사이즈만큼 데이터를 받고, 해당 발신지를 from_adr에 저장
			
			if( errno == EINTR && read_cnt == -1) //만약 타임아웃이 났다면 다시 서버로 부터 받음
			{
				while(1)
				{
					send_str_len = sendto(sd, init, BUF_SIZE,0,(struct sockaddr*)&serv_adr,sizeof(serv_adr));
					//다시 재전송을 요구하는 패킷을 보냄
					if( send_str_len == -1 )
					{
						//printf("ack send error from client\n");
						continue;
					}
					else
					{
						//printf("ack 를 보냈음.(client->server)");
						alarm(0); //타이머 값 설정
						break;
					}
				}
				continue;
			}	
			
			//아에 패킷 받는것을 시스템 적으로 실패했을 때는 다시 보내달라고 요청
			if( read_cnt == -1 )
			{
				//printf("client에서 패킷을 받지 못함( 재전송 요망)\n");
				continue;
			}
			else
			{
				//패킷 받는 것을 성공했다면 계속 진행
				/*
				//memset(chk_recv,0,sizeof(chk_recv));
				//strcpy(chk_recv, buf);
				//buf 출력
				//printf("Buf 내용물 출력 \n");
				//for( int i=0; i<BUF_SIZE; ++i )
				{
					////printf("%02x",buf[i]);
				}
				printf("\n\n");
				*/
				break;	
			}
		}
		
		for(int i=0; i<HEADER_SIZE; ++i ) //헤더분리
		{
			header[i] = recv_buf[i];
		}
		for( int i=HEADER_SIZE; i<BUF_SIZE; ++i) //payload 분리
		{
			data[i-HEADER_SIZE]=recv_buf[i];
		}
			
		server_seq = atoi(header); //server에서 보낸 seq를 저장
			
		if( server_seq != client_seq ) //클라이언트에서 받을 시작 위치와 서버에서 보낸 위치가 맞지 않으면 재전송 요망
		{
			//printf("server seq : %d   ,    client_seq : %d( 재전송 요망)\n", server_seq, client_seq);			memset( init, 0, sizeof(init) );
			strcpy( init, "resend" );

			while(1)
			{	
				send_str_len = sendto(sd, init, BUF_SIZE,0,(struct sockaddr*)&serv_adr,sizeof(serv_adr));
				//재전송을 요구하는 패킷 전송
				if( send_str_len == -1 )
				{
					//printf("resend 요청 실패, 다시 전송\n");
					continue;
				}
				else
				{
					//printf("resend 요청 성공\n");
					break;	
				}
			}
			continue;
		}
	
		
	
		//순서도 맞고, 패킷도 잘 왔으면 파일에 데이터 기록
		fwrite((void*)data,1,read_cnt-HEADER_SIZE,fp);
	
		//클라이언트가 현재 받은 패킷수를 갱신
		client_seq = server_seq + read_cnt - HEADER_SIZE - 1;//여기까지 받았고.
		
		/*
		//printf("client에서 %d~%d 까지 받음\n", server_seq, client_seq);//여기까지 받았고
		for( int i=0; i<BUF_SIZE; ++i )
		{
			//printf("%02x",chk_recv[i]);
		}
		//printf("\n\n\n");
		*/
		++client_seq; // 클라이언트가 다음에 받을 데이터 시퀀스를 갱신
		
		memset(init,0,sizeof(init));
		sprintf(init, "%d", client_seq);   //잘받았다는 사인인, 다음번 받을 seq전달

		while(1)
		{
			//ack를 보내줌
			send_str_len = sendto(sd, init, BUF_SIZE,0,(struct sockaddr*)&serv_adr,sizeof(serv_adr));
			if( send_str_len == -1 )
			{
				//printf("ack send error from client\n");
				continue;
			}
			else
			{
				//printf("ack 를 보냈음.(client->server)");
				break;
			}
		}
		
		if( read_cnt < BUF_SIZE) //전송받을 데이터를 다 받으면 loop 종료
			break;
	}
	

	printf("client end\n"); //데이터를 다 받았다면 파일 포인터와 소켓을 닫아줌
	fclose(fp);
	close(sd);
	return 0;
}

void ErrorHandling(char *message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}