#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
//���� ���α׷����� ���� ������� ����

#define BUF_SIZE 1024 
//��Ŷ ������ ���� ���� ����� 1024�� ����

void error_handling(char *message){ //���� �޽����� ������� �Լ�
	fputs(message, stderr);
	fputc('\n',stderr);
	exit(1);
}

int main(int argc, char *argv[]){
	int serv_sd, clnt_sd; //������ ����, ������ ��ȯ�� �̿�Ȱ accept�� �����Ǵ� ���� ������ ���� ���� ����
	char buf[BUF_SIZE]; //�����͸� ���� ���۸� ����
	int read_cnt; //�����Ϳ��� ������ �������� ������ ���� ����
	
	struct sockaddr_in serv_adr; //�� ���Ͽ� ���� �ּ� ������ ���� ���� ����
	struct sockaddr_in clnt_adr;
	
	socklen_t clnt_adr_sz; //clnt_adr�� ũ�⸦ ������ ���� ����
	FILE * fp; //���� ������ �б� ���� ���� ������ ���� ����
	
	if(argc!=2){ //�ش� ��Ʈ���� �� ������ �Ǿ����� Ȯ���ϴ� �ϼ�
		printf("Usage : %s <port>\n",argv[0]);
		exit(1);
	}

	fp = fopen("lec11.pdf","rb"); //lec11.pdf�� ������ �����ϱ� ����, ���� �����ͷ� ���̳ʸ� ���·� �о� �´�.
	serv_sd = socket( PF_INET, SOCK_STREAM, 0 ); //IPv4������ ���������̰�, tcp ��Ʈ�� ����� ���� ������ ����
	
	if(serv_sd==-1) //���� ������ �������� Ȯ���� ���� �Լ�
		error_handling("socket() error");

	memset(&serv_adr,0,sizeof(serv_adr)); //�ּ� ������ ���� ����ü ���� �ʱ�ȭ
	
	serv_adr.sin_family=AF_INET; //IPv4�ּ��ǰ踦 �̿��ϱ� ������ ����
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //������ ����ϴ� ��ǻ���� ip�ּ� �ڵ� �Ҵ�
	serv_adr.sin_port=htons(atoi(argv[1]));//�ش��ϴ� ��Ʈ�� ���� ���� ����

	if(bind(serv_sd,(struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error"); //��Ʈ�� ������ �� ���� �Ǿ����� Ȯ��
	if(listen(serv_sd,5)==-1)//listen�Լ��� ��û�� ��ٸ���, �ش� ��û�� �������� Ȯ��
		error_handling("listen() error");
		
	clnt_adr_sz=sizeof(clnt_adr); //clnt_adr �� ������ ����
	clnt_sd=accept(serv_sd,(struct sockaddr*)&clnt_adr,&clnt_adr_sz); //accept�� ���� ��û������ Ŭ���̾�Ʈ�� ��Ʈ�� ���� ����
		
	if(clnt_sd==-1) //��Ʈ�� ������ �������� Ȯ��
		error_handling("accept() error");
	else
		printf("Connected client 1 \n");
	
	while(1)
	{
		read_cnt = fread((void*)buf,1,BUF_SIZE, fp); //���� �����͸� �̿��Ͽ� ���̳ʸ� �����͸� �о��
		if( read_cnt < BUF_SIZE){ //������ ������ buf ������� �۰� ���������� �׸�ŭ�� �о���̰� �극��ũ�� ��������
			write(clnt_sd,buf,read_cnt);
			break;
		}		
		write(clnt_sd,buf,BUF_SIZE); //���� �����ŭ �����͸� ����
	}
	
	shutdown(clnt_sd,SHUT_WR);//��Ʈ�� ������ ���ɸ� �ݾ���(���� ���ϸ� �ݾ��ְ�, �д� ������ ����� -> Ŭ���̾�Ʈ�� ������ �޽��� Ȯ���� ���Ͽ�)
	read(clnt_sd,buf,BUF_SIZE);//Ŭ���̾�Ʈ���� ������ Ȯ�� �޽��� Ȯ��
	printf("Message from client:%s \n",buf);
		
	fclose(fp); //���� ������ �������Ƿ� , ���� �����͸� �ݾ���
	close(clnt_sd);//��Ʈ�� ���� �ݾ���
	close(serv_sd);//���� ������ �ݾ���
	return 0;
}