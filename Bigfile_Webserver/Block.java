package webserver;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousFileChannel;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

import org.apache.log4j.Logger;

/**
 *  실제 파일 정보를 담고 있는 버퍼 역할을 하는 블럭 
 */
public class Block {
	int blockNum;	// 블럭 번호
	long offset;	// 이 블럭이 전송하려는 파일의 어느 위치부터 담고 있는지를 표시
	int blockSize;	// 블럭의 크기
	
	public byte[] buffer;			// 실제 파일 정보를 담고 있는 바이트 배열 버퍼
	public int readSize;			// 버퍼 안에 실제로 담고 있는 크기
	
	public Integer refCount = 0;	// 이 블럭을 참조하고 있는 레퍼런스 카운트 수
	
	Logger logger = Logger.getLogger(Block.class);
	
	public Block(int blockNum, final int BLOCK_SIZE) {
		this.blockNum = blockNum;
		this.offset = (long)blockNum * (long)BLOCK_SIZE;
		this.blockSize = BLOCK_SIZE;
	}

	/**
	 * 블럭의 레퍼런스 카운트를 증가, 감소시킬 때는 스레드에 안전하게 수행한다
	 */
	public synchronized void increaseRefCount() {
		refCount += 1;
	}
	
	public synchronized void decreaseRefCount() {
		refCount -= 1;
		
		/**
		 * 레퍼런스 카운트 수가 0이 되면 아무도 이 블럭을 참조하고 있지 않는 것이므로
		 * 블럭을 메모리에서 비워낸다
		 */
		if (refCount == 0) {
			buffer = null;
		}
	}

	/**
	 *	 버퍼에 해당 블럭이 메모리에 로드되어 있지 않을 경우,
	 *	 File I/O를 해서 메모리로 로드함.
	 */
	public void load(String filePath) {
		Path path = Paths.get(filePath);
		AsynchronousFileChannel fc = null;
		try {
			/**
			 * 서로 다른 블럭을 여러 스레드가 동시에 파일을 로드할 수 있으므로
			 * 비동기 파일 I/O를 사용함.
			 */
			fc = AsynchronousFileChannel.open(path, StandardOpenOption.READ);
			ByteBuffer buf = ByteBuffer.allocate(blockSize);
			Future<Integer> result = fc.read(buf, offset);
			
			/**
			 * Future.get() 메소드를 호출하면 이전에 수행하던 파일 I/O가
			 * 끝날 때까지 대기함.
			 */
			this.readSize = result.get();
			buf.flip();
			this.buffer = buf.array();
		} catch (IOException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		} catch (ExecutionException e) {
			e.printStackTrace();
		} finally {
			if (fc != null) {
				try {
					fc.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
	}
	
	@Override
	public String toString() {
		return "[ " + blockNum + " : " + refCount + " ]";
	}
}
