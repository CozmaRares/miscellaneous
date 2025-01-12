import { env } from "@/env";
import mongoose from "mongoose";

let isConnected = false;

async function connect() {
  if (isConnected) {
    return;
  }

  await mongoose.connect(env.MONGO_URI, {
    serverApi: { version: "1", strict: true, deprecationErrors: true },
  });

  isConnected = true;
}

export async function mongoQuery<T>(callback: () => Promise<T>): Promise<T> {
  try {
    await connect();
    return await callback();
  } catch (err) {
    console.error("Mongo error", err);
    throw err;
  }
}
