package webserver;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.ThreadPoolExecutor;

import org.apache.log4j.Logger;

public class Monitor implements Runnable {
	Logger logger = Logger.getLogger(Monitor.class);
	
	/**
	 * 모니터링의 대상이 되는 스레드 풀과
	 * 블럭 단위의 버퍼 배열
	 */
	ThreadPoolExecutor monitoredPool;
	Block[] blocks;
	
	int delay;
	private boolean isRun = true; 
	
	public Monitor(ExecutorService monitoredPool, Block[] blocks, int delay) {
		this.monitoredPool = (ThreadPoolExecutor)monitoredPool; 
		this.blocks = blocks;
		this.delay = delay;
	}
	
	@Override
	public void run() {
		while (isRun) {
			/**
			 * 스레드 풀의 Pool Size와 Active Count를 모니터링
			 * 버퍼 안의 각 블럭의 레퍼런스 카운트 갯수를 모니터링
			 */
			logger.info("Pool Size : " + monitoredPool.getPoolSize() + " / Active : " + monitoredPool.getActiveCount());
			for (Block block : blocks) {
				logger.info(block);
			}
			
			try {
				Thread.sleep(1000 * delay);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}
}
