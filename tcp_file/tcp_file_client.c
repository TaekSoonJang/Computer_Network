/*���� ���α׷����� ���� ������� ����*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <time.h> 
//avi������ ������ �޴´�. �ð����� ���ϸ��� �ٸ��� �ϱ� ���� time �Լ��� ��.
//time �Լ��� ���� ��� ����


//��Ŷ�� ���� ���ۻ������ 1024�� ����
#define BUF_SIZE 1024

//���� �޽��� ����� ���� �Լ�
void ErrorHandling(char *message);

int main(int argc, char *argv[]){
	int sd;			//Ŭ���̾�Ʈ���� �����ϴ� ����
	FILE *fp;		//Ŭ���̾�Ʈ���� �������� ���� ������ write �� ������ ���� ������
	char buf[BUF_SIZE];//Ŭ���̾�Ʈ���� ��Ŷ�� ������ ���� ����
	int read_cnt; //���ۿ��� ���� ������ ����.
	struct sockaddr_in serv_adr; //���� ������ ������ ���� �ּҸ� ���� ����ü
	
	if(argc!=3){ //IP�� ��Ʈ�� ���ڷ� �ް� �ش� �ּҷ� connect ��û
		printf("Usage: %s <IP> <port>\n",argv[0]);
		exit(1);
	}
	
	
	time_t current_time; //���� �ð��� ����� �ֱ� ���� �ð��� ������ ����
	time( &current_time);	 //������ �ð��� ����
	char file_name[BUF_SIZE]; //FILE NAME�� ������ ���� �ּ�
	sprintf(file_name, "%d.pdf",current_time);//������ ������ "����ð�.pdf"�� ����
	printf("%s\n",file_name); //�ش��ϴ� ���� �̸��� Ŭ���̾�Ʈ���� ���
	
	fp = fopen(file_name,"wb"); //�ش� ���Ͽ��ٰ� �������� ���� �����͸� ������.
	sd =socket(PF_INET,SOCK_STREAM,0); //IPv4 �������� ü���̸� TCP��Ʈ������ ������ �����Ͽ� endpoint ����
	
	if(sd==-1) //���� ������ �����ߴ��� üũ
		ErrorHandling("socket() error");

	memset(&serv_adr,0,sizeof(serv_adr)); //Ŀ��Ʈ�Ϸ��� ������ ���� �ּҸ� ���� ����ü�� �ʱ�ȭ
	serv_adr.sin_family=AF_INET; //IPv4�� �ּ�ü�踦 ����ϱ� ������ AF_INET���
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);// htonl�� �ʿ���� �ڵ����� "10.73.43.49" ������ �Է��� ip�ּҿ� ���� 32bit�� �ּҷ� ���� ��ȯ�� ���ش�.
										            // ����Ʈ �������� �ڵ����� ���ֱ� ������ htonl�� �ʿ� ����.
	serv_adr.sin_port=htons(atoi(argv[2])); //�����Ϸ��� ��Ʈ ��ȣ�� ����(atoi�� ���ڷ� ��ȯ ��, htons�� ����Ʈ �������� ��Ʈ��ũ�� �������� ���߾���).

	if(connect(sd,(struct sockaddr*)&serv_adr,sizeof(serv_adr))==-1) //�ش� ������ Ŀ��Ʈ ��û
		ErrorHandling("connect() error!");//Ŀ��Ʈ ���� üũ
	else
		puts("Conneected......\n"); //Ŀ��Ʈ ������ Ȯ���ϱ� ���� ��� �޼���

	while((read_cnt=read(sd,buf,BUF_SIZE))!=0) //Ŀ��Ʈ�� ������ �Ǹ� �ش� ��Ʈ���� �̿��Ͽ� �������� ���� ������ ����(���̴� read_cnt��ŭ ����)
	{
		fwrite((void*)buf,1,read_cnt,fp); //buf�� ��� payload�� Ŭ���̾�Ʈ�� file pointer�� ���ʴ�� ���(����)
		//printf("������ ���� ��Ŷ ����.\n");
	}
		
	puts("Recieved file data"); //file ������ ������ Ȯ���ϱ� ���� �޽��� ���
	fclose(fp);//���� �����͸� �ݾ���
	close(sd);//�ش� ������ �ݾ��ְ� ���� ����
	return 0;
}

void ErrorHandling(char *message) //���� �޽��� ����� ���� �Լ�
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}