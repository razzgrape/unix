import os
import json
import asyncio
from loguru import logger
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from aiokafka import AIOKafkaProducer

KAFKA_BOOTSTRAP_SERVERS = os.getenv("KAFKA_BOOTSTRAP_SERVERS", "kafka:29092")
TASK_TOPIC = "tasks_topic"

app = FastAPI()
producer = None

class TimeSeriesData(BaseModel):
    symbol: str
    timestamp: str
    value: float

@app.on_event("startup")
async def startup_event():
    global producer
    retry_count = 0
    max_retries = 25

    while retry_count < max_retries:
        try:
            logger.info(f"Attempting to connect to Kafka ({retry_count + 1} / {max_retries})")
            producer = AIOKafkaProducer(
                bootstrap_servers=KAFKA_BOOTSTRAP_SERVERS,
                value_serializer=lambda v: json.dumps(v).encode('utf-8')
            )
            await producer.start()
            logger.info("Producer connected successfully")
            return 
        except Exception as e:
            logger.warning(f"Connection attempt failed: {e}")
            retry_count += 1
            await asyncio.sleep(5)
        
    logger.critical("Could not connect to Kafka after multiple retries")
    raise Exception("Could not connect to Kafka after multiple retries")

@app.on_event("shutdown")
async def shutdown_event():
    if producer:
        await producer.stop()
        logger.info("Kafka producer stopped successfully")

@app.post("/data",status_code=202)  
async def ingest_data(data: TimeSeriesData):
    try:
        await producer.send_and_wait(TASK_TOPIC, data.model_dump())
        
        return {
            "status": "accepted",
            "symbol": data.symbol,
            "message": "Data received and successfully queued for analysis"
        }
    
    except Exception as e:
        logger.error(f"Failed to send message to kafka: {e}")
        raise HTTPException(
            status_code=503,
            detail="Service unavailable. failed to connect to message broker"
        )
    