#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
/*
udp�� ��Ŷ�ν��� ���� Ÿ�Ӿƿ��� �ɾ
recvfrom�� ���ŷ�� �� ������ �ذ��ϱ� ����
��� �κ� ����
*/
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
/*
udp���� ��Ŷ�ν��� ���� üũ�� ����, tcp���� �ϴ� ������
�����̰� Ÿ�Ӿƿ� ��Ʈ�����̼��� �����ϱ� ���Ͽ� ��������� ������ ���� �߰�
udp�� ���۴� ������� 1024�� ����
���� 16����Ʈ�� ��� ������ ���� 1008 ������� �����Ͱ� ��
*/
#define BUF_SIZE 1024
#define HEADER_SIZE 16
#define DATA_SIZE 1008

static const unsigned int TIMEOUT_SECS = 2; //������ ��ٸ��� �ð�
static const unsigned int MAXTRIES = 5;// ������ ������ �ݺ��ϴ� Ƚ��
unsigned int tries = 0;//���� Ƚ��( �ñ׳� �ڵ鷯�� ������ ���� �������� ó��)

void CatchAlarm( int ignored ) //Ÿ�Ӿƿ��� ���� ��� ��õ� �ϴ� Ƚ���� ����
{
	tries += 1;
}//SIGALRM �ڵ鷯


void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n',stderr);
	exit(1);
}

int main(int argc, char *argv[]){
	int serv_sd, clnt_sd; //���� ������ ���� ���� ����
	
	char buf[BUF_SIZE], temp[DATA_SIZE],header[BUF_SIZE], data[DATA_SIZE], init[DATA_SIZE], recv_buf[BUF_SIZE],send_buf[BUF_SIZE];
	//������ ���� ���ۻ�����, ó�� udp�� ������ �˸��� init ����, ����� payload�� ���� ���� �� ������ ����, �޴� ���۸� ���� ���� ����
	
	int read_cnt, send_str_len,recv_str_len;
	//������ ���� ����, ���� ������ ������ ���� ���� ����
	
	struct sockaddr_in serv_adr;
	struct sockaddr_in clnt_adr;
	
	//���� ���� ������, ���κ��� �Դ��� ������ �ּҸ� ���� ���� ����
	socklen_t clnt_adr_sz;	// �ּ��� ������ ������ ���� ���� ����
	FILE * fp; //�а��� �ϴ� ������ �о���� ���� ���� ������ ����
	

	
	
	//  IP�� ��Ʈ���� ���� ��������Ȯ���� ���� �Լ�
	if(argc!=2){
		printf("Usage : %s <port>\n",argv[0]);
		exit(1);
	}

	//���� ������ �б� �������� ����, ���������Ϳ� ����
	fp = fopen("lec11.pdf","rb");
	serv_sd = socket( PF_INET, SOCK_DGRAM, 0 );//IPv4�������� ü��� udp �� ������ �׷� ������ ������ ����
	
	if(serv_sd==-1)//���� ���� ���� Ȯ��
		error_handling("socket() error");

	memset(&serv_adr,0,sizeof(serv_adr));//�ּ������� ���� ����ü �ʱ�ȭ
	
	serv_adr.sin_family=AF_INET;//IPv4 �ּ� ü�� �̿�
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //�ش� IP�ּҸ� ����
	serv_adr.sin_port=htons(atoi(argv[1]));//��Ʈ ������ ����

	//���ϰ� ��Ʈ ������ ����
	if(bind(serv_sd,(struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
		
	
	//ó������ �߽����� �𸣹Ƿ�, �̴� ��Ŷ�� �ϳ� ����
	clnt_adr_sz=sizeof(clnt_adr);
	recv_str_len = recvfrom(serv_sd, buf, BUF_SIZE,0,(struct sockaddr*)&clnt_adr,&clnt_adr_sz);

	/*
		��Ŷ �ν��� ���� Ÿ�Ӿƿ� ������ ���Ͽ� �ñ׳� �Լ��� ���.
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
	
	//�ش� Ÿ�Ӿƿ��� �ð��� ����
	alarm(TIMEOUT_SECS); //Ÿ�̸� �� ����
	
	int server_seq = 1, client_seq = 0;//�������� ���� ���� ���� ,client_seq : client�� ���� �� ���� ��Ŷ�� ����, server_seq : ������ ���� ��Ŷ ����
	
	while(1)
	{
		//�ش� ������ ���� ���۵��� �ʱ�ȭ
		memset(buf,0,sizeof(buf));
		memset(header,0,sizeof(header));
		memset(data,0,sizeof(data));
		memset(temp,0,sizeof(temp));
		
		read_cnt = fread((void*)temp,1,DATA_SIZE, fp); //temp���ۿ� ������ �����ŭ �о�ͼ� �����͸� ����

		sprintf( buf, "%d", server_seq ); //������ 16���̿� �ش��ϴ� ���ʿ��� ������ ������ ������ �ѹ��� ��������� �־���
		
		if( read_cnt < DATA_SIZE ) //�������� �ϴ� ������ ������ �κ��̶�� �ش� ������ �����ְ� ����
		{ 
			for( int i=HEADER_SIZE; i<HEADER_SIZE+read_cnt; ++i )
			{
				buf[i] = temp[i-HEADER_SIZE]; //������ �����Ͱ� ������ �κп� temp�� ��Ƶξ��� ������ ����
			}
	
			while(1)
			{
				int flag = 0; //�������� Ȯ���ϴ� flag ����
				while(1)
				{
					send_str_len = sendto(serv_sd, buf, HEADER_SIZE+read_cnt, 0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
					//Ŭ���̾�Ʈ�� �ش� ���۸� ����
					if( send_str_len == -1 )
					{
						//printf("�������� ��Ŷ�� �������� ������ \n");
						continue;
					}
					else 
						break;
				}
				
				while(1)
				{
					alarm(TIMEOUT_SECS); //Ÿ�̸� �� ����
					recv_str_len = recvfrom(serv_sd, recv_buf, BUF_SIZE,0,(struct sockaddr*)&clnt_adr,&clnt_adr_sz);
					//Ŭ���̾�Ʈ�κ��� ack�� �� �޾Ҵ��� Ȯ��
					//���� Ÿ�Ӿƿ��� ���� �ٽ� �������� ��û.
					if( errno == EINTR && recv_str_len == -1)
					{
						printf("server time out!!");
						flag = 1;
						alarm(0); //Ÿ�̸� �� ����
						break;
					}
					if( recv_str_len == -1 )
					{
						//printf( "client�κ��� seq������ \n");
						continue;
					}
					else
						break;
				}
				if( flag )
					continue;//Ÿ�Ӿƿ��� ���ٸ� �ٽ� ���ΰ��� �������� ��.
				
				if( !strcmp(recv_buf,"resend") ) //���� resend��� ���� �Դٸ� ������.
				{
					continue;
				}
				else //�� �޾�����
				{	
					client_seq = atoi(recv_buf); //���� ������� ������ �ѹ��� ���� , ����
					//printf("������ �κ� %d�������� %d���� ��Ŷ�� ���� \n",server_seq, client_seq -1 );
					server_seq = client_seq; //�������� ���� ������ �ѹ� ����
					break;
				}
			}
			//���� ��
			break;
		}
		
		
		for( int i=HEADER_SIZE; i<HEADER_SIZE+DATA_SIZE; ++i )
		{
			buf[ i ] = temp[ i-HEADER_SIZE ];//������ �����Ͱ� ������ �κп� temp�� ��Ƶξ��� ������ ����
		}
		
		while(1)
		{
			int flag = 0;//�������� Ȯ���ϴ� flag ����
			
			while(1)
			{
				send_str_len = sendto(serv_sd, buf, BUF_SIZE, 0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
				//Ŭ���̾�Ʈ�� �ش� ���۸� ����
				if( send_str_len == -1 )
				{
				//	printf("�������� ��Ŷ�� �������� ������ \n");
					continue;
				}
				else 
				{
					//memset(chk_send,0,sizeof(chk_send));
		//			printf("client�� ��Ŷ ���� ������\n");
					//strcpy(chk_send, buf);
					break;
				}
			}
			
			while(1)
			{
				alarm(TIMEOUT_SECS); //Ÿ�̸� �� ����
				recv_str_len = recvfrom(serv_sd, recv_buf, BUF_SIZE,0,(struct sockaddr*)&clnt_adr,&clnt_adr_sz);
				//Ŭ���̾�Ʈ�κ��� ack�� �� �޾Ҵ��� Ȯ��
				//���� Ÿ�Ӿƿ��� ���� �ٽ� �������� ��û
				if ( errno == EINTR && recv_str_len == -1)
				{
					printf("server time out\n");
					flag = 1;
					alarm(0); //Ÿ�̸� �� ����
			//		printf(" intrrupt server recv \n");
					break;
				}
				if( recv_str_len == -1 )
				{
			//		printf( "client�κ��� seq������ \n");
					continue;
				}
				else
				{
			//		printf("client�κ��� seq�� ����\n");
					//strcpy(chk_recv, buf);
					break;
				}
			}
			if( flag == 1)//Ÿ�Ӿƿ��� ���ٸ� �ٽ� ���ΰ��� �������� ��.
				continue;
				
			if( !strcmp(recv_buf,"resend") )//���� resend��� ���� �Դٸ� ������.
			{
		//		printf(" resend ���ּ��� from server \n");
				continue;
			}
			else //�� �޾�����
			{	
				client_seq = atoi(recv_buf);//���� ������� ������ �ѹ��� ���� , ����
				/*
			//	printf("%d �������� %d���� ��Ŷ�� ���� \n",server_seq, client_seq -1 );
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
	
	alarm(0); //Ÿ�Ӿƿ� ���� ����
	
	printf("server end\n"); //�������� ������ �������� �˸��� �޽��� ���
	fclose(fp);// ���� �����͸� ����
	close(serv_sd);//������ ����
	return 0;
}