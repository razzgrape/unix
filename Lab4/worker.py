import os
import json
import asyncio
import signal
import socket
import random
from loguru import logger
from aiokafka import AIOKafkaConsumer

KAFKA_BOOTSTRAP_SERVERS = os.getenv("KAFKA_BOOTSTRAP_SERVERS", "kafka:29092")
TASK_TOPIC = os.getenv("TASK_TOPIC", "tasks_topic")

CONSUMER_GROUP_ID = os.getenv("CONSUMER_GROUP_ID", "financial_analysis_workers")
WORKER_ID = socket.gethostname()

shutdown_event = asyncio.Event()

def handle_sigterm():
    logger.warning("SIGTERM received — finishing current task before shutdown...")
    shutdown_event.set()

async def process_task(task: dict, worker_id: str): 
    symbol = task.get("symbol", "N/A")
    value = task.get("value", 0.0)

    logger.info(f'{worker_id} starting analysis for {symbol} (Value: {value})')

    analysis_time = random.uniform(2.0, 5.0)
    await asyncio.sleep(analysis_time)

    if value > 100:
        status = "ANOMALY DETECTED"
        logger.warning(f"[{worker_id}] ALERT: Value {value} for {symbol} is above threshold 100")
    else:
        status  = "Normal"
    
    logger.info(f"[{worker_id}] analysis finished in {analysis_time:.2f}s. Result: {status}\n")

async def run_worker():
    loop = asyncio.get_running_loop()

    loop.add_signal_handler(signal.SIGTERM, handle_sigterm)
    loop.add_signal_handler(signal.SIGINT, handle_sigterm)

    logger.info(f'Worker [{WORKER_ID}] starting up')

    consumer = None
    retry_count = 0
    max_retries = 25
    
    while retry_count < max_retries:
        try:
            logger.info(f"Worker [{WORKER_ID}] connecting to kafka ({retry_count + 1}/{max_retries})")
            
            consumer = AIOKafkaConsumer(
                TASK_TOPIC,
                bootstrap_servers=KAFKA_BOOTSTRAP_SERVERS,
                group_id=CONSUMER_GROUP_ID,
                auto_offset_reset = 'latest',
                enable_auto_commit=False,
                value_deserializer=lambda x: json.loads(x.decode('utf-8'))
            )
            await consumer.start()
            logger.info(f"Worker [{WORKER_ID}] successfully connected and listening to topic '{TASK_TOPIC}'")
            break
        except Exception as e:
            logger.warning(f"Worker connection failed: {e}")
            retry_count += 1
            await asyncio.sleep(5)
    
    if not consumer:
        logger.critical(f"Critical: Worker [{WORKER_ID}] could not connect. Exiting.")
        return
    
    try:
        while not shutdown_event.is_set():
            msg = await consumer.getone()

            task = msg.value

            await process_task(task, WORKER_ID)

            await consumer.commit()
    
    except asyncio.CancelledError:
        logger.warning(f"Worker [{WORKER_ID}] main loop cancelled")
    finally:
        await consumer.stop()
        logger.info(f"Worker [{WORKER_ID}] gracefully stopped")

if __name__ == "__main__":
    asyncio.run(run_worker())



