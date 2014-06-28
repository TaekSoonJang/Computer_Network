package webserver;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Scanner;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.log4j.Logger;

public class WebServer {
	/**
	 * 상수 정의
	 */
	static int HEADER_SIZE = 1024;
	static int BLOCK_SIZE = 100000000;	// 100mb
	static int NUM_OF_THREADS = 100;
	
	Logger logger = Logger.getLogger(WebServer.class);
	
	ServerSocket ss;
	
	/**
	 * 파일에 관련된 변수들
	 * @see		실제 구현에서는 이렇게 static하게 받으면 안되고 요청에 맞게 설정해야 할듯.
	 */
	final static String FILE_PATH = "/Users/TaekSoon/Downloads/network-big/target.mp4";
	final long fileSize;
	final int numOfBlocks;
	
	/**
	 * 하나의 파일을 미리 정해둔 BLOCK_SIZE 단위의 블럭으로 분할한다.
	 * 각 블럭들이 모두 모이면 하나의 파일을 이룸.
	 * 각 블럭의 파일 읽기 내용을 배열로 관리하는 변수.
	 */
	Block[] blocks;
	
	public WebServer(int port) {
		/**
		 * 파일의 정보를 받아서 적당한 블럭의 갯수를 미리 계산
		 * @see		이 웹서버는 static하게 하나의 파일만을 서비스하지만
		 * 			실제 구현체에서는 요청을 받은 뒤 그 요청에 맞는 파일의 정보를 받아오는
		 * 			순서로 구현해야 할 듯.
		 */
		File file = new File(FILE_PATH);
		this.fileSize = file.length();
		this.numOfBlocks = (int)(this.fileSize / (long)BLOCK_SIZE) + 1;
		
		/**
		 * 위에서 계산한 정보를 토대로 블럭 단위의 버퍼 배열을 초기화함.
		 */
		blocks = new Block[this.numOfBlocks];
		for (int i = 0; i < numOfBlocks; i++) {
			blocks[i] = new Block(i, BLOCK_SIZE);
		}
		
		/**
		 * 서버 소켓 생성
		 */
		try {
			this.ss = new ServerSocket(port);
			logger.info("server socket created. [ " + this.ss.getLocalSocketAddress() + " ]");
		} catch (IOException e) {
			logger.error("ServerSocket creation failed.");
			e.printStackTrace();
		}
	}
	
	/**
	 * 클라이언트 요청을 받는 메서드
	 */
	public void accept() {
		/**
		 * 커넥션 풀링
		 */
		ExecutorService pool = Executors.newFixedThreadPool(NUM_OF_THREADS);
		
		/**
		 * 파일 I/O 버퍼의 상태를 주기적으로 체크하는 모니터 스레드 실행
		 */
		Monitor monitor = new Monitor(pool, blocks, 10);
		Thread monitorThread = new Thread(monitor);
		monitorThread.start();
		
		while (true) {
			try {
				/**
				 * 클라이언트 요청 accept
				 */
				logger.info("accepting....");
				final Socket socket = ss.accept();
				
				/**
				 * 요청 내용을 파싱해서 올바른 요청인지 분석
				 * @see		정적 파일 하나만을 서비스 하므로 그 외의 요청엔 응답하지 않음. 
				 */
				DataInputStream dis = new DataInputStream(socket.getInputStream());
				logger.info("Server is listening to [ address : " + socket.getRemoteSocketAddress() + ", port : " + socket.getLocalPort());
				String request = pasreHeader(dis);
				Scanner scanner = new Scanner(request);
				String method = scanner.next();
				String uri = scanner.next();
				scanner.close();
				if ("GET".equals(method) && "/target.mp4".equals(uri)) {
					logger.info("Good Request");
					
					/**
					 * 올바른 요청일 경우 스레드를 생성해서 요청을 처리
					 */
					pool.execute(new Runnable() {
						@Override
						public void run() {
							handleEvent(socket);
						}
					});
				} else {
					logger.info("methodLine : " + method + " " + uri);
				}
			} catch (IOException e) {
				logger.error("accept failed.");
				e.printStackTrace();
			}
		}
	}
	
	/**
	 *	파일을 전송하는 메서드 
	 */
	private void handleEvent(Socket socket) {
		try {
			/**
			 * 헤더 정보를 먼저 전송
			 */
			StringBuilder sb = new StringBuilder();
			sb.append("HTTP/1.1 200 OK\r\nContent-Type:application/force-download\r\n");
			sb.append("Content-Length:" + fileSize + "\r\n\r\n");
			String header = sb.toString();
			logger.info(header);
			DataOutputStream dos = new DataOutputStream(socket.getOutputStream());
			dos.write(header.getBytes());
			logger.info("header sent.");
			
			/**
			 * 블럭의 배열로 구성된 버퍼를 순차적으로 탐색하면서 파일 전송.
			 * 이미 메모리에 올라와 있는 블럭은 바로 네트워크로 전송하고,
			 * 그렇지 않은 경우 메모리에 새로 올림.
			 * 레퍼런스 카운트를 조정하는 부분은 스레드에 안전하게 동기화.
			 */
			for (int blockIdx = 0; blockIdx < numOfBlocks; blockIdx++) {
				synchronized (this) {
					if (blocks[blockIdx].refCount == 0) {
						logger.info("loading file data.... (block num : " + blockIdx + ")");
						blocks[blockIdx].load(FILE_PATH);
					}
				}
				blocks[blockIdx].increaseRefCount();
				dos.write(blocks[blockIdx].buffer, 0, blocks[blockIdx].readSize);
				blocks[blockIdx].decreaseRefCount();
			}
			
			logger.info("closing socket...");
			socket.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * 요청 헤더를 파싱하는 메서드
	 */
	private String pasreHeader(DataInputStream dis) {
		String content = null;
		try {
			byte[] buffer = new byte[HEADER_SIZE];
			dis.read(buffer);
			content = new String(buffer);
			logger.info(content);
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			return content;
		}
	}
	
	/**
	 * 테스트 함수 
	 */
	public static void main(String[] args) {
		WebServer server = new WebServer(5000);
		server.accept();
	}
}
